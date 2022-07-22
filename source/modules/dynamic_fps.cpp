/*
 * Copyright (C) 2021-2022 Matt Yang
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "dynamic_fps.h"
#include "cobridge_type.h"
#include "utils/fmt_exception.h"
#include "utils/misc.h"
#include "utils/misc_android.h"
#include <spdlog/spdlog.h>

constexpr char MODULE_NAME[] = "DynamicFps";
constexpr int64_t DEFAULT_GESTURE_SLACK_MS = 4000;
constexpr int64_t DEFAULT_TOUCH_SLACK_MS = 4000;
constexpr bool DEFAULT_USE_SF_BACKDOOR = false;
constexpr int MIN_TOUCH_SLACK_MS = 100;
constexpr char UNIVERSIAL_PKG_NAME[] = "*";
constexpr char OFFSCREEN_PKG_NAME[] = "-";

std::string Trim(const std::string &str) {
    if (str.empty()) {
        return str;
    }
    auto firstScan = str.find_first_not_of(" \n\r");
    auto first = (firstScan == std::string::npos) ? str.length() : firstScan;
    auto last = str.find_last_not_of(" \n\r");
    return str.substr(first, last - first + 1);
}

DynamicFps::DynamicFps(const std::string &configPath, const std::string &notifyPath)
    : useSfBackdoor_(DEFAULT_USE_SF_BACKDOOR),
      touchSlackMs_(DEFAULT_TOUCH_SLACK_MS),
      gestureSlackMs_(DEFAULT_GESTURE_SLACK_MS),
      hasUniversial_(false),
      hasOffscreen_(false),
      notifyPath_(notifyPath),
      touchPressed_(false),
      btnPressed_(false),
      active_(false),
      isOffscreen_(false),
      curHz_(INT32_MAX),
      forceSwitch_(false),
      dwInput_(DwCreate(MODULE_NAME)),
      dwGesture_(DwCreate(MODULE_NAME)),
      dwWakeup_(DwCreate(MODULE_NAME)),
      hw_(HwCreate(MODULE_NAME)) {
    LoadConfig(configPath);
}

void DynamicFps::Start(void) { AddReactor(); }

void DynamicFps::LoadConfig(const std::string &configPath) {
    FILE *fp = fopen(configPath.c_str(), "re");
    if (fp == NULL) {
        throw FmtException("Cannot open config '{}'", configPath);
    }

    char buf[256];
    while (feof(fp) == false) {
        buf[0] = '\0';
        fgets(buf, sizeof(buf), fp);
        auto line = Trim(buf);
        ParseLine(line);
    }

    if (hasOffscreen_ == false) {
        fclose(fp);
        throw FmtException("Offscreen rule not specified in the config file");
    }
    if (hasUniversial_ == false) {
        fclose(fp);
        throw FmtException("Default rule not specified in the config file");
    }
    auto invalidRuleName = FindInvalidRule();
    if (invalidRuleName.empty() == false) {
        fclose(fp);
        throw FmtException("Rule of '{}' is invalid", invalidRuleName);
    }

    if (useSfBackdoor_) {
        SPDLOG_INFO("Use surfaceflinger backdoor to switch refresh rate");
    } else {
        SPDLOG_INFO("Use PEAK_REFRESH_RATE to switch refresh rate");
    }

    fclose(fp);
}

void DynamicFps::ParseLine(const std::string &line) {
    auto isComment = [](const std::string &line) { return line[0] == '#'; };
    auto isTunable = [](const std::string &line) { return line[0] == '/'; };

    char name[256];
    char value[256];
    name[0] = '\0';
    value[0] = '\0';

    if (line.empty() || isComment(line)) {
        return;
    } else if (isTunable(line)) {
        // /touchSlackMs 4000
        if (sscanf(line.c_str(), "/%s %s", name, value) == 2) {
            SPDLOG_DEBUG("Set '{}'={}", name, value);
            SetTunable(name, value);
        } else {
            SPDLOG_WARN("Skipped broken line '{}'", line);
        }
    } else {
        // com.example.app 60 120
        // com.example.app 2 0
        FpsRule rule;
        if (sscanf(line.c_str(), "%s %d %d", name, &rule.idle, &rule.active) == 3) {
            AddRule(name, rule);
        } else {
            SPDLOG_WARN("Skipped broken line '{}'", line);
        }
    }
}

void DynamicFps::AddRule(const std::string &pkgName, FpsRule rule) {
    auto isUniversial = [](const std::string &pkgName) { return pkgName == UNIVERSIAL_PKG_NAME; };
    auto isOffscreen = [](const std::string &pkgName) { return pkgName == OFFSCREEN_PKG_NAME; };
    if (isUniversial(pkgName)) {
        hasUniversial_ = true;
        universial_ = rule;
    } else if (isOffscreen(pkgName)) {
        hasOffscreen_ = true;
        offscreen_ = rule;
    } else {
        SPDLOG_DEBUG("Load '{}', dfps={}/{}", pkgName, rule.idle, rule.active);
        rules_.emplace(pkgName, rule);
    }
}

void DynamicFps::SetTunable(const std::string &tunable, const std::string &value) {
    if (tunable == "useSfBackdoor") {
        useSfBackdoor_ = (std::stoi(value) > 0) ? true : false;
    } else if (tunable == "touchSlackMs") {
        touchSlackMs_ = std::max(MIN_TOUCH_SLACK_MS, std::stoi(value));
    } else {
        SPDLOG_WARN("Unknown tunable '{}' in the config file", tunable);
    }
}

std::string DynamicFps::FindInvalidRule(void) {
    auto isDefaultRule = [](const FpsRule &rule) { return rule.idle == -1 && rule.active == -1; };
    auto isSfBackdoorRule = [](const FpsRule &rule) { return rule.idle < 20 && rule.active < 20; };
    auto isInvalid = [=](const FpsRule &rule) {
        return isDefaultRule(rule) == false && useSfBackdoor_ != isSfBackdoorRule(rule);
    };

    if (isInvalid(offscreen_)) {
        return "offscreen";
    }
    if (isInvalid(universial_)) {
        return "default";
    }
    for (const auto &[name, rule] : rules_) {
        if (isInvalid(rule)) {
            return name;
        }
    }

    return {};
}

void DynamicFps::AddReactor(void) {
    using namespace std::placeholders;
    CoSubscribe("input.touch", std::bind(&DynamicFps::OnInputTouch, this, _1));
    CoSubscribe("input.btn", std::bind(&DynamicFps::OnInputBtn, this, _1));
    CoSubscribe("input.state", std::bind(&DynamicFps::OnInputScene, this, _1));
    CoSubscribe("topapp.pkgName", std::bind(&DynamicFps::OnTopAppSwitch, this, _1));
    CoSubscribe("offscreen.state", std::bind(&DynamicFps::OnOffscreen, this, _1));
}

void DynamicFps::OnInputTouch(const void *data) {
    const auto &pressed = CoBridge::Get<bool>(data);
    touchPressed_ = pressed;
    OnInput();
}

void DynamicFps::OnInputBtn(const void *data) {
    const auto &pressed = CoBridge::Get<bool>(data);
    btnPressed_ = pressed;
    OnInput();
}

void DynamicFps::OnInput(void) {
    auto pressed = touchPressed_ || btnPressed_;
    if (pressed) {
        active_ = true;
        SwitchRefreshRate();
        DwSetWork(dwInput_, nullptr, DelayedWorker::SLEEP_TS);
    } else {
        auto enterIdle = [=]() {
            active_ = false;
            SwitchRefreshRate();
        };
        DwSetWork(dwInput_, enterIdle, GetNowTs() + MsToUs(touchSlackMs_));
    }
}

void DynamicFps::OnInputScene(const void *data) {
    const auto &input = CoBridge::Get<InputData>(data);
    if (input.inGesture) {
        overridedApp_ = UNIVERSIAL_PKG_NAME;
        SwitchRefreshRate();
        DwSetWork(dwGesture_, nullptr, DelayedWorker::SLEEP_TS);
    } else {
        auto resume = [=]() {
            if (overridedApp_ == UNIVERSIAL_PKG_NAME) {
                overridedApp_ = "";
                SwitchRefreshRate();
            }
        };
        DwSetWork(dwGesture_, resume, GetNowTs() + MsToUs(gestureSlackMs_));
    }
}

void DynamicFps::OnTopAppSwitch(const void *data) {
    const auto &topApp = CoBridge::Get<std::string>(data);
    if (topApp != curApp_) {
        curApp_ = topApp;
        forceSwitch_ = true;
        SwitchRefreshRate();
    }
}

void DynamicFps::OnOffscreen(const void *data) {
    const auto &isOff = CoBridge::Get<bool>(data);
    if (isOff == isOffscreen_) {
        return;
    }

    isOffscreen_ = isOff;
    if (isOff) {
        overridedApp_ = OFFSCREEN_PKG_NAME;
        forceSwitch_ = true;
        SwitchRefreshRate();
    } else {
        auto exitOffscreen = [=]() {
            if (overridedApp_ == OFFSCREEN_PKG_NAME) {
                overridedApp_ = "";
                forceSwitch_ = true;
                SwitchRefreshRate();
            }
        };
        DwSetWork(dwWakeup_, exitOffscreen, GetNowTs() + MsToUs(gestureSlackMs_));
    }
}

void DynamicFps::SwitchRefreshRate(void) {
    FpsRule rule;
    const auto &pkgName = overridedApp_.empty() ? curApp_ : overridedApp_;
    if (pkgName == OFFSCREEN_PKG_NAME) {
        rule = offscreen_;
    } else {
        auto it = rules_.find(pkgName);
        if (it != rules_.end()) {
            rule = it->second;
        } else {
            rule = universial_;
        }
    }
    int hz = active_ ? rule.active : rule.idle;
    HwSetWork(hw_, [=]() { SwitchRefreshRate(hz); });
}

void DynamicFps::SwitchRefreshRate(int hz) {
    auto force = forceSwitch_;
    forceSwitch_ = false;
    SPDLOG_DEBUG("switch {}", hz);
    if (force == false && hz == curHz_) {
        return;
    }

    std::string hzStr = std::to_string(hz);
    curHz_ = hz;
    NotifyRefreshRate(hzStr);
    if (useSfBackdoor_) {
        SysSurfaceflingerBackdoor(hzStr, force);
    } else {
        SysPeakRefreshRate(hzStr, force);
    }
}

void DynamicFps::NotifyRefreshRate(const std::string_view &hz) {
    int fd = open(notifyPath_.c_str(), O_WRONLY | O_NONBLOCK | O_CLOEXEC | O_CREAT | O_TRUNC);
    if (fd > 0) {
        WriteFile(fd, hz);
        close(fd);
    }
}
