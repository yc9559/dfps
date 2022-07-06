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

#include "offscreen_monitor.h"
#include "utils/misc.h"
#include "utils/misc_android.h"
#include <spdlog/spdlog.h>

// constexpr char MODULE_NAME[] = "OffscreenMonitor";
constexpr int RESTRICTED_TASK_NR_MIN = 10;

OffscreenMonitor::OffscreenMonitor() : prevOffscreen_(false) {}

OffscreenMonitor::~OffscreenMonitor() {}

void OffscreenMonitor::Start(void) {
    using namespace std::placeholders;
    CoSubscribe("cgroup.re.list", std::bind(&OffscreenMonitor::OnRestrictedList, this, _1));
}

void OffscreenMonitor::OnRestrictedList(const void *data) {
    auto pids = CoBridge::Get<PidList>(data);
    bool isOffscreen = pids.size() > RESTRICTED_TASK_NR_MIN;
    if (isOffscreen != prevOffscreen_) {
        prevOffscreen_ = isOffscreen;
        SPDLOG_DEBUG("offscreen.state {}", isOffscreen);
        CoPublish("offscreen.state", &isOffscreen);
    }
}
