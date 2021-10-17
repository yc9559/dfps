/*
Dfps
Copyright (C) 2021 Matt Yang(yccy@outlook.com)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "cgroup_listener.h"
#include "utils/cobridge.h"
#include "utils/misc.h"
#include <unistd.h>

constexpr char MODULE_NAME[] = "CgroupListener";
constexpr int WATCH_FLAG = Inotify::MODIFY;
const std::string TOP_PATH = "/dev/cpuset/top-app/tasks";
const std::string FG_PATH = "/dev/cpuset/foreground/tasks";
const std::string BG_PATH = "/dev/cpuset/background/tasks";
const std::string RE_PATH = "/dev/cpuset/restricted/tasks";

constexpr int64_t INOTIFY_EVT_MIN_INTERVAL_US = 10 * 1000;
constexpr int64_t SCAN_CGROUP_MIN_INTERVAL_MS = 100;

CgroupListener::CgroupListener() {
    using namespace std::placeholders;
    dw_ = DelayedWorker::GetInstance()->Create(MODULE_NAME);
    hw_ = HeavyWorker::GetInstance()->Create(MODULE_NAME);

    inoti_.Add(TOP_PATH, WATCH_FLAG, std::bind(&CgroupListener::OnTaModified, this, _1, _2));
    inoti_.Add(FG_PATH, WATCH_FLAG, std::bind(&CgroupListener::OnFgModified, this, _1, _2));
    inoti_.Add(BG_PATH, WATCH_FLAG, std::bind(&CgroupListener::OnBgModified, this, _1, _2));
    inoti_.Add(RE_PATH, WATCH_FLAG, std::bind(&CgroupListener::OnReModified, this, _1, _2));
}

void CgroupListener::Start(void) {
    th_ = std::thread([this]() {
        SetSelfThreadName(MODULE_NAME);
        for (;;) {
            inoti_.WaitAndHandle();
            usleep(INOTIFY_EVT_MIN_INTERVAL_US);
        }
    });
    th_.detach();
}

void CgroupListener::OnTaModified(const std::string &, int) {
    CoBridge::GetInstance()->Publish("cgroup.ta.update", nullptr);
    UpdatePidList();
}

void CgroupListener::OnFgModified(const std::string &, int) {
    CoBridge::GetInstance()->Publish("cgroup.fg.update", nullptr);
    UpdatePidList();
}

void CgroupListener::OnBgModified(const std::string &, int) {
    CoBridge::GetInstance()->Publish("cgroup.bg.update", nullptr);
    UpdatePidList();
}

void CgroupListener::OnReModified(const std::string &, int) {
    CoBridge::GetInstance()->Publish("cgroup.re.update", nullptr);
    UpdatePidList();
}

void CgroupListener::UpdatePidList(void) {
    if (pidListTimer_.ElapsedMs() < SCAN_CGROUP_MIN_INTERVAL_MS) {
        return;
    }
    pidListTimer_.Reset();

    auto delayed = [this]() {
        auto heavywork = [this]() {
            auto updatePidList = [](PidList *pl, const std::string &path) {
                GetUnsignedIntFromFile(path, pl);
                std::sort(pl->begin(), pl->end());
            };
            updatePidList(&ta_, TOP_PATH);
            CoBridge::GetInstance()->Publish("cgroup.ta.list", &ta_);
            updatePidList(&fg_, FG_PATH);
            CoBridge::GetInstance()->Publish("cgroup.fg.list", &ta_);
            updatePidList(&bg_, BG_PATH);
            CoBridge::GetInstance()->Publish("cgroup.bg.list", &ta_);
            updatePidList(&re_, RE_PATH);
            CoBridge::GetInstance()->Publish("cgroup.re.list", &ta_);
        };
        HeavyWorker::GetInstance()->SetWork(hw_, heavywork);
    };
    DelayedWorker::GetInstance()->SetWork(dw_, delayed, GetNowTs() + MsToUs(SCAN_CGROUP_MIN_INTERVAL_MS));
}
