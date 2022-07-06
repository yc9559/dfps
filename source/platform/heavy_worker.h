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

#include "singleton.h"
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class HeavyWorker : public Singleton<HeavyWorker> {
public:
    using Handle = int;
    using Work = std::function<void(void)>;

    HeavyWorker();
    Handle Create(const std::string &name);
    void SetWork(Handle handle, const Work &work);

private:
    std::vector<std::tuple<std::string, Work>> works_;
    bool pending_;
    std::mutex mut_;
    std::condition_variable cv_;
    std::thread th_;
};
