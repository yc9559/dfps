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

#include "platform/cobridge.h"
#include "platform/delayed_worker.h"
#include "platform/heavy_worker.h"

class ModuleBase {
public:
    ModuleBase();
    virtual ~ModuleBase();
    virtual void Start(void) = 0;

protected:
    void CoPublish(const std::string &theme, const void *data) const;
    void CoSubscribe(const std::string &theme, const CoBridge::SubscribeCallback &cb);
    bool CoHasSubscriber(const std::string &theme) const;

    DelayedWorker::Handle DwCreate(const std::string &name);
    void DwSetWork(DelayedWorker::Handle handle, const DelayedWorker::Work &work, int64_t ts);

    HeavyWorker::Handle HwCreate(const std::string &name);
    void HwSetWork(HeavyWorker::Handle handle, const HeavyWorker::Work &work);

    bool disabled_;
};
