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

constexpr int WATCH_FLAG = Inotify::MODIFY;
const std::string TOP_PATH = "/dev/cpuset/top-app/tasks";
const std::string FG_PATH = "/dev/cpuset/foreground/tasks";
const std::string BG_PATH = "/dev/cpuset/background/tasks";
const std::string RE_PATH = "/dev/cpuset/restricted/tasks";

constexpr int64_t INOTIFY_EVT_MIN_INTERVAL_US = 10 * 1000;
constexpr int64_t SCAN_CGROUP_MIN_INTERVAL_MS = 100;

CgroupListener::CgroupListener() {
    using namespace std::placeholders;
    inoti_.Add(TOP_PATH, WATCH_FLAG, std::bind(&CgroupListener::OnTaModified, this, _1, _2));
    inoti_.Add(FG_PATH, WATCH_FLAG, std::bind(&CgroupListener::OnFgModified, this, _1, _2));
    inoti_.Add(BG_PATH, WATCH_FLAG, std::bind(&CgroupListener::OnBgModified, this, _1, _2));
    inoti_.Add(RE_PATH, WATCH_FLAG, std::bind(&CgroupListener::OnReModified, this, _1, _2));
}

void CgroupListener::Start(void) {
    th_ = std::thread([this]() {
        for (;;) {
            inoti_.WaitAndHandle();
            usleep(INOTIFY_EVT_MIN_INTERVAL_US);
        }
    });
    th_.detach();
}

void UpdatePidList(PidList *pl, const std::string &path) {
    GetUnsignedIntFromFile(path, pl);
    std::sort(pl->begin(), pl->end());
}

void CgroupListener::OnTaModified(const std::string &, int) {
    CoBridge::GetInstance()->Publish("cgroup.ta.update", nullptr);
    if (taTimer_.ElapsedMs() > SCAN_CGROUP_MIN_INTERVAL_MS) {
        taTimer_.Reset();
        UpdatePidList(&ta_, TOP_PATH);
        CoBridge::GetInstance()->Publish("cgroup.ta.list", &ta_);
    }
}

void CgroupListener::OnFgModified(const std::string &, int) {
    CoBridge::GetInstance()->Publish("cgroup.fg.update", nullptr);
    if (fgTimer_.ElapsedMs() > SCAN_CGROUP_MIN_INTERVAL_MS) {
        fgTimer_.Reset();
        UpdatePidList(&fg_, FG_PATH);
        CoBridge::GetInstance()->Publish("cgroup.fg.list", &fg_);
    }
}

void CgroupListener::OnBgModified(const std::string &, int) {
    CoBridge::GetInstance()->Publish("cgroup.bg.update", nullptr);
    if (bgTimer_.ElapsedMs() > SCAN_CGROUP_MIN_INTERVAL_MS) {
        bgTimer_.Reset();
        UpdatePidList(&bg_, BG_PATH);
        CoBridge::GetInstance()->Publish("cgroup.bg.list", &bg_);
    }
}

void CgroupListener::OnReModified(const std::string &, int) {
    CoBridge::GetInstance()->Publish("cgroup.re.update", nullptr);
    if (reTimer_.ElapsedMs() > SCAN_CGROUP_MIN_INTERVAL_MS) {
        reTimer_.Reset();
        UpdatePidList(&re_, RE_PATH);
        CoBridge::GetInstance()->Publish("cgroup.re.list", &re_);
    }
}
