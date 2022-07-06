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

#include "cgroup_listener.h"
#include "platform/inotifier.h"
#include "utils/atrace.h"
#include "utils/misc.h"
#include <scn/scn.h>
#include <spdlog/spdlog.h>
#include <unistd.h>

constexpr char MODULE_NAME[] = "CgroupListener";
constexpr char TOP_PATH[] = "/dev/cpuset/top-app/tasks";
constexpr char FG_PATH[] = "/dev/cpuset/foreground/tasks";
constexpr char BG_PATH[] = "/dev/cpuset/background/tasks";
constexpr char RE_PATH[] = "/dev/cpuset/restricted/tasks";

constexpr size_t CGROUP_TASKS_MAX_LEN = 32 * 1024;
constexpr int64_t SCAN_DELAY_LONG_MS = 80;  // read tasklist cost 3~8ms
constexpr int64_t SCAN_DELAY_SHORT_MS = 20; // wait process create threads

CgroupListener::Updater::Updater(const std::string &path, const std::string &topic, int64_t delayMs, bool heavy)
    : hw_(HeavyWorker::GetInstance()->Create(MODULE_NAME)),
      dw_(DelayedWorker::GetInstance()->Create(MODULE_NAME)),
      path_(path),
      topic_(topic),
      delayUs_(MsToUs(delayMs)),
      heavy_(heavy) {}

void CgroupListener::Updater::Start(void) {
    if (CoBridge::GetInstance()->HasSubscriber(topic_) == false) {
        return;
    }

    // Do not start delayed work too frequently
    if (tm_.ElapsedMs() < 1) {
        return;
    }
    tm_.Reset();

    auto delayed = [this]() {
        if (heavy_) {
            HeavyWorker::GetInstance()->SetWork(hw_, [this]() { ReadAndPub(); });
        } else {
            ReadAndPub();
        }
    };
    DelayedWorker::GetInstance()->SetWork(dw_, delayed, GetNowTs() + delayUs_);
}

void CgroupListener::Updater::ReadAndPub(void) {
    AtraceScopeHelper __ash(topic_);

    std::string buf;
    pl_.clear();
    auto len = ReadFile(path_, &buf, CGROUP_TASKS_MAX_LEN);
    if (len < 0) {
        return;
    }
    scn::scan_list(buf, pl_, '\n');

    CoBridge::GetInstance()->Publish(topic_, &pl_);
}

CgroupListener::CgroupListener()
    : ta_(TOP_PATH, "cgroup.ta.list", SCAN_DELAY_SHORT_MS, false),
      fg_(FG_PATH, "cgroup.fg.list", SCAN_DELAY_LONG_MS, true),
      bg_(BG_PATH, "cgroup.bg.list", SCAN_DELAY_LONG_MS, true),
      re_(RE_PATH, "cgroup.re.list", SCAN_DELAY_LONG_MS, false) {}

CgroupListener::~CgroupListener() {}

void CgroupListener::Start(void) {
    // some device don't have background cpuset
    auto addWatch = [=](const std::string &path, auto callback) {
        auto inoti = Inotifier::GetInstance();
        try {
            inoti->Add(path, Inotify::MODIFY, callback);
        } catch (const std::exception &e) {
            SPDLOG_WARN("{}", e.what());
        }
    };

    using namespace std::placeholders;
    addWatch(TOP_PATH, std::bind(&CgroupListener::OnTaModified, this, _1, _2));
    addWatch(FG_PATH, std::bind(&CgroupListener::OnFgModified, this, _1, _2));
    addWatch(BG_PATH, std::bind(&CgroupListener::OnBgModified, this, _1, _2));
    addWatch(RE_PATH, std::bind(&CgroupListener::OnReModified, this, _1, _2));
}

void CgroupListener::OnTaModified(const std::string &, int) {
    CoPublish("cgroup.ta.update", nullptr);
    ta_.Start();
    fg_.Start();
    bg_.Start();
}

void CgroupListener::OnFgModified(const std::string &, int) {
    CoPublish("cgroup.fg.update", nullptr);
    ta_.Start();
    fg_.Start();
    bg_.Start();
    re_.Start();
}

void CgroupListener::OnBgModified(const std::string &, int) {
    CoPublish("cgroup.bg.update", nullptr);
    ta_.Start();
    fg_.Start();
    bg_.Start();
}

void CgroupListener::OnReModified(const std::string &, int) {
    CoPublish("cgroup.re.update", nullptr);
    fg_.Start();
    re_.Start();
}
