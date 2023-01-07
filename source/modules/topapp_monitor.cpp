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

#include "topapp_monitor.h"
#include "utils/atrace.h"
#include "utils/misc.h"
#include "utils/misc_android.h"
#include <spdlog/spdlog.h>

constexpr char MODULE_NAME[] = "TopappMonitor";
constexpr int64_t TOP_APP_SWITCH_DELAY_MS = 800;
constexpr size_t TOP_TASK_NR_DIFF_MIN = 10;

TopappMonitor::TopappMonitor() : topappNr_(0), hw_(HwCreate(MODULE_NAME)), dw_(DwCreate(MODULE_NAME)) {}

TopappMonitor::~TopappMonitor() {}

void TopappMonitor::Start(void) {
    using namespace std::placeholders;
    if(GetOSVersion() >= 13) {
        std::thread android13([this](){
            pthread_setname_np(pthread_self(),"Android13Worker");
            SPDLOG_INFO("Android13Worker: GetOSVer: {}",GetOSVersion());
            while(true)
            {
                ATRACE_SCOPE(GetTopAppName);
                auto pkgName = GetTopAppNameDumpsys();
                if (pkgName.empty()) {
                    sleep(1);
                    continue;
                }
                if (pkgName != prevPkgName_) {
                    prevPkgName_ = pkgName;
                    SPDLOG_DEBUG("topapp.pkgName {}", pkgName);
                    CoPublish("topapp.pkgName", &pkgName);
                }
                sleep(1);
            }
        });
        android13.detach();
    }
    CoSubscribe("cgroup.ta.list", std::bind(&TopappMonitor::OnTopappList, this, _1));
}

void TopappMonitor::OnTopappList(const void *data) {
    if (CoHasSubscriber("topapp.pkgName") == false) {
        return;
    }

    const auto &pl = CoBridge::Get<PidList>(data);
    auto nr = static_cast<int>(pl.size());
    if (std::abs(nr - topappNr_) <= TOP_TASK_NR_DIFF_MIN) {
        return;
    }
    topappNr_ = nr;

    auto delayed = [this]() {
        auto heavywork = [this]() {
            ATRACE_SCOPE(GetTopAppName);
            auto pkgName = GetTopAppNameDumpsys();
            if (pkgName.empty()) {
                return;
            }
            if (pkgName != prevPkgName_) {
                prevPkgName_ = pkgName;
                SPDLOG_DEBUG("topapp.pkgName {}", pkgName);
                CoPublish("topapp.pkgName", &pkgName);
            }
        };
        HwSetWork(hw_, heavywork);
    };
    DwSetWork(dw_, delayed, GetNowTs() + MsToUs(TOP_APP_SWITCH_DELAY_MS));
}
