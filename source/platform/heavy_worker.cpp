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

#include "heavy_worker.h"
#include "utils/fmt_exception.h"
#include "utils/misc.h"

constexpr char MODULE_NAME[] = "HeavyWorker";

HeavyWorker::HeavyWorker() : pending_(false) {
    th_ = std::thread([this]() mutable {
        SetSelfThreadName(MODULE_NAME);
        Work work;
        for (;;) {
            {
                std::unique_lock<std::mutex> lk(mut_);
                cv_.wait(lk, [this]() { return pending_; });
                pending_ = false;
            }
            for (auto &[name, w] : works_) {
                {
                    std::lock_guard<std::mutex> lk(mut_);
                    if (w == nullptr) {
                        continue;
                    }
                    work = w;
                    w = nullptr;
                }
                work();
            }
        }
    });
    th_.detach();
}

HeavyWorker::Handle HeavyWorker::Create(const std::string &name) {
    Handle handle = works_.size();
    works_.emplace_back(std::make_tuple(name, nullptr));
    return handle;
}

void HeavyWorker::SetWork(Handle handle, const Work &work) {
    if (handle < 0 || handle >= works_.size()) {
        throw FmtException("HeavyWorker handle not found, handle = {}", handle);
    }

    std::unique_lock<std::mutex> lk(mut_);
    auto &[_1, w] = works_[handle];
    w = work;
    if (work) {
        pending_ = true;
        lk.unlock();
        cv_.notify_one();
    }
}
