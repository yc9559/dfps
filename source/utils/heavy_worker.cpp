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

#include "heavy_worker.h"
#include "fmt_exception.h"
#include "misc.h"

constexpr HeavyWorker::Handle EMPTY_HANDLE = -1;

HeavyWorker::HeavyWorker() {
    th_ = std::thread([this]() mutable {
        Work work;
        Handle cur;
        Handle prevHandle = EMPTY_HANDLE;
        for (;;) {
            {
                std::unique_lock<std::mutex> lk(mut_);
                cv_.wait(lk, [this]() {
                    for (const auto &[_1, w] : works_) {
                        if (w) {
                            return true;
                        }
                    }
                    return false;
                });
            }
            cur = EMPTY_HANDLE;
            for (auto &[name, w] : works_) {
                cur++;
                {
                    std::lock_guard<std::mutex> lk(mut_);
                    if (w == nullptr) {
                        continue;
                    }
                    work = w;
                    w = nullptr;
                }
                if (cur != prevHandle) {
                    SetSelfThreadName(name);
                }
                prevHandle = cur;
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

    auto &[_1, w] = works_[handle];
    std::unique_lock<std::mutex> lk(mut_);
    w = work;
    if (work) {
        lk.unlock();
        cv_.notify_one();
    }
}
