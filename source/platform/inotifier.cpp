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

#include "inotifier.h"
#include "utils/misc.h"

constexpr char MODULE_NAME[] = "Inotifier";
constexpr int64_t INOTIFY_EVT_MIN_INTERVAL_MS = 10;

Inotifier::Inotifier() {
    th_ = std::thread([this]() {
        SetSelfThreadName(MODULE_NAME);
        for (;;) {
            inoti_.WaitAndHandle();
            Sleep(MsToUs(INOTIFY_EVT_MIN_INTERVAL_MS));
        }
    });
    th_.detach();
}

void Inotifier::Add(const std::string &path, int mask, Inotify::Notifier notifier) {
    return inoti_.Add(path, mask, notifier);
}
