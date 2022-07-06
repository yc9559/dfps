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

#pragma once

#include "platform/module_base.h"
#include <map>
#include <string>

class DynamicFps : public ModuleBase {
public:
    explicit DynamicFps(const std::string &configPath);
    void Start(void);

private:
    struct FpsRule {
        int idle;
        int active;
    };

    void LoadConfig(const std::string &configPath);
    void ParseLine(const std::string &line);
    void AddRule(const std::string &pkgName, FpsRule rule);
    void SetTunable(const std::string &tunable, const std::string &value);
    std::string FindInvalidRule(void);

    void AddReactor(void);
    void OnInputTouch(const void *data);
    void OnInputBtn(const void *data);
    void OnInput(void);
    void OnInputScene(const void *data);
    void OnTopAppSwitch(const void *data);
    void OnOffscreen(const void *data);

    void SwitchRefreshRate(void);
    void SwitchRefreshRate(int hz);

    bool useSfBackdoor_;
    int64_t touchSlackMs_;
    int64_t gestureSlackMs_;

    std::map<std::string, FpsRule> rules_;
    FpsRule offscreen_;
    FpsRule universial_;
    bool hasUniversial_;
    bool hasOffscreen_;

    bool touchPressed_;
    bool btnPressed_;
    bool active_;
    std::string curApp_;
    std::string overridedApp_;
    bool isOffscreen_;
    int curHz_;
    bool forceSwitch_;

    DelayedWorker::Handle dwInput_;
    DelayedWorker::Handle dwGesture_;
    DelayedWorker::Handle dwWakeup_;
    HeavyWorker::Handle hw_;
};
