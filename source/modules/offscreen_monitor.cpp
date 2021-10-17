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

#include "offscreen_monitor.h"
#include "utils/cobridge.h"
#include "utils/misc.h"
#include "utils/misc_android.h"
#include <spdlog/spdlog.h>

// constexpr char MODULE_NAME[] = "OffscreenMonitor";
constexpr int RESTRICTED_TASK_NR_MIN = 10;

OffscreenMonitor::OffscreenMonitor() : prevOffscreen_(false) {}

void OffscreenMonitor::Start(void) {
    using namespace std::placeholders;
    auto co = CoBridge::GetInstance();
    co->Subscribe("cgroup.re.list", std::bind(&OffscreenMonitor::OnRestrictedList, this, _1));
}

void OffscreenMonitor::OnRestrictedList(const void *data) {
    auto pids = CoBridge::Get<PidList>(data);
    bool isOffscreen = pids.size() > RESTRICTED_TASK_NR_MIN;
    if (isOffscreen != prevOffscreen_) {
        prevOffscreen_ = isOffscreen;
        SPDLOG_DEBUG("offscreen.state {}", isOffscreen);
        CoBridge::GetInstance()->Publish("offscreen.state", &isOffscreen);
    }
}
