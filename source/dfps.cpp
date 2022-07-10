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

#include "dfps.h"
#include "modules/cgroup_listener.h"
#include "modules/dynamic_fps.h"
#include "modules/input_listener.h"
#include "modules/offscreen_monitor.h"
#include "modules/topapp_monitor.h"
#include "utils/sched_ctrl.h"
#include <spdlog/spdlog.h>

Dfps::Dfps() {}

Dfps::~Dfps() {}

void Dfps::Load(const std::string &configPath) { configPath_ = configPath; }

void Dfps::Start(void) {
    SetSelfSchedHint();

    modules_.emplace_back(std::make_unique<InputListener>());
    modules_.emplace_back(std::make_unique<CgroupListener>());
    modules_.emplace_back(std::make_unique<TopappMonitor>());
    modules_.emplace_back(std::make_unique<OffscreenMonitor>());
    modules_.emplace_back(std::make_unique<DynamicFps>(configPath_));

    for (const auto &m : modules_) {
        m->Start();
    }
    SPDLOG_INFO("Dfps is running");
}

void Dfps::SetSelfSchedHint(void) {
    // heavyworker prio 120
    SchedCtrlSetStaticPrio(0, 120, false);
    HeavyWorker::GetInstance();
    // others prio 98
    SchedCtrlSetStaticPrio(0, 98, false);
}
