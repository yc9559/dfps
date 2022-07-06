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

#include "delayed_worker.h"
#include "utils/fmt_exception.h"
#include "utils/misc.h"
#include <signal.h>
#include <unistd.h>

constexpr char MODULE_NAME[] = "DelayedWorker";

void OnTimerWrapper(union sigval v) { DelayedWorker::GetInstance()->_OnTimer(); }

DelayedWorker::DelayedWorker() : timerId_(nullptr), isThreadInited_(false) {
    constexpr int SIVAL_INT_DEFAULT = 111;
    struct sigevent evp;
    memset(&evp, 0, sizeof(struct sigevent));
    evp.sigev_value.sival_int = SIVAL_INT_DEFAULT;
    evp.sigev_notify = SIGEV_THREAD;
    evp.sigev_notify_function = OnTimerWrapper;
    timer_create(CLOCK_MONOTONIC, &evp, &timerId_);
}

DelayedWorker::~DelayedWorker() { timer_delete(timerId_); }

DelayedWorker::Handle DelayedWorker::Create(const std::string &name) {
    std::lock_guard<std::mutex> lk(mut_);
    Handle handle = slots_.size();
    slots_.emplace_back(std::make_tuple(SLEEP_TS, nullptr));
    return handle;
}

void DelayedWorker::SetWork(Handle handle, const Work &work, int64_t ts) {
    if (handle < 0 || handle >= slots_.size()) {
        throw FmtException("DelayedWorker handle not found, handle = {}", handle);
    }

    {
        std::lock_guard<std::mutex> lk(mut_);
        auto &[preserveTs, w] = slots_[handle];
        w = work;
        preserveTs = ts;
        if (work == nullptr) {
            preserveTs = SLEEP_TS;
        }
        ReloadTimer();
    }
}

void DelayedWorker::_OnTimer(void) {
    if (isThreadInited_ == false) {
        isThreadInited_ = true;
        SetSelfThreadName(MODULE_NAME);
    }

    Work work;
    {
        std::lock_guard<std::mutex> lk(mut_);
        auto idx = FindEarliestReadyWorkIdx();
        if (idx >= 0) {
            auto &[preserveTs, w] = slots_[idx];
            preserveTs = SLEEP_TS;
            if (w) {
                work = w;
            }
        }
    }

    // SetWork() may be called in work()
    if (work) {
        work();
    }

    {
        std::lock_guard<std::mutex> lk(mut_);
        ReloadTimer();
    }
}

void DelayedWorker::ReloadTimer(void) {
    constexpr int64_t MIN_INTERVAL = 1;
    auto setTimer = [this](int64_t usec) {
        struct itimerspec itv;
        itv.it_value.tv_sec = usec / 1000000ull;
        itv.it_value.tv_nsec = (usec % 1000000ull) * 1000ull;
        itv.it_interval.tv_sec = 0;
        itv.it_interval.tv_nsec = 0;
        return timer_settime(timerId_, 0, &itv, NULL);
    };

    auto idx = FindEarliestWorkIdx();
    if (idx >= 0) {
        // if preserveTs is earlier than now, work also need to be done
        auto now = GetNowTs();
        const auto &[preserveTs, w] = slots_[idx];
        auto delta = (preserveTs > now) ? (preserveTs - now) : MIN_INTERVAL;
        setTimer(delta);
    } else {
        setTimer(0);
    }
}

int DelayedWorker::FindEarliestWorkIdx(void) {
    int64_t nextTs = INT64_MAX;
    int idx = -1;
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        const auto &[preserveTs, w] = slots_[i];
        if (preserveTs > SLEEP_TS && preserveTs < nextTs) {
            nextTs = preserveTs;
            idx = i;
        }
    }
    return idx;
}

int DelayedWorker::FindEarliestReadyWorkIdx(void) {
    auto now = GetNowTs();
    int64_t nextTs = INT64_MAX;
    int idx = -1;
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        const auto &[preserveTs, w] = slots_[i];
        if (preserveTs > SLEEP_TS && preserveTs < nextTs && preserveTs < now) {
            nextTs = preserveTs;
            idx = i;
        }
    }
    return idx;
}
