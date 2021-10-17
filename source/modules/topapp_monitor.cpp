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

#include "topapp_monitor.h"
#include "utils/cobridge.h"
#include "utils/misc.h"
#include "utils/misc_android.h"

constexpr char MODULE_NAME[] = "TopappMonitor";
constexpr int64_t TOP_APP_SWITCH_DELAY_MS = 1000;
constexpr int TOP_TASK_NR_DIFF_MIN = 10;

TopappMonitor::TopappMonitor() : curTopTaskNr_(0), prevTopTaskNr_(0) {
    dw_ = DelayedWorker::GetInstance()->Create(MODULE_NAME);
    hw_ = HeavyWorker::GetInstance()->Create(MODULE_NAME);
}

void TopappMonitor::Start(void) {
    using namespace std::placeholders;
    auto co = CoBridge::GetInstance();
    co->Subscribe("cgroup.ta.update", std::bind(&TopappMonitor::OnCgroupUpdated, this, _1));
    co->Subscribe("cgroup.fg.update", std::bind(&TopappMonitor::OnCgroupUpdated, this, _1));
    co->Subscribe("cgroup.bg.update", std::bind(&TopappMonitor::OnCgroupUpdated, this, _1));
    co->Subscribe("cgroup.ta.list", std::bind(&TopappMonitor::OnTopappList, this, _1));
}

void TopappMonitor::OnCgroupUpdated(const void *data) {
    if (timer_.ElapsedMs() < TOP_APP_SWITCH_DELAY_MS) {
        return;
    }
    timer_.Reset();

    auto delayed = [this]() {
        int delta = std::abs(curTopTaskNr_ - prevTopTaskNr_);
        prevTopTaskNr_ = curTopTaskNr_;
        if (delta < TOP_TASK_NR_DIFF_MIN) {
            return;
        }
        auto heavywork = []() {
            auto pkgName = GetTopAppNameDumpsys();
            if (pkgName.empty()) {
                return;
            }
            CoBridge::GetInstance()->Publish("topapp.pkgName", &pkgName);
        };
        HeavyWorker::GetInstance()->SetWork(hw_, heavywork);
    };
    DelayedWorker::GetInstance()->SetWork(dw_, delayed, GetNowTs() + MsToUs(TOP_APP_SWITCH_DELAY_MS));
}

void TopappMonitor::OnTopappList(const void *data) {
    auto pids = CoBridge::Get<PidList>(data);
    curTopTaskNr_ = pids.size();
}
