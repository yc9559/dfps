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
#include <functional>
#include <time.h>
#include <vector>

class DelayedWorker : public Singleton<DelayedWorker> {
public:
    using Handle = int;
    using Work = std::function<void(void)>;
    static constexpr int64_t SLEEP_TS = 0;

    DelayedWorker();
    ~DelayedWorker();
    Handle Create(const std::string &name);
    void SetWork(Handle handle, const Work &work, int64_t ts);

    void _OnTimer(void);

private:
    void ReloadTimer(void);
    int FindEarliestWorkIdx(void);
    int FindEarliestReadyWorkIdx(void);

    std::vector<std::tuple<int64_t, Work>> slots_;
    std::mutex mut_;
    timer_t timerId_;
    bool isThreadInited_;
};
