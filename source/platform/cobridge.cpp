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

#include "cobridge.h"

CoBridge::CoBridge() {}

void CoBridge::Publish(const std::string &theme, const void *data) const {
    auto it = pubsub_.find(theme);
    if (it != pubsub_.end()) {
        for (const auto &cb : it->second) {
            cb(data);
        }
    }
}

void CoBridge::Subscribe(const std::string &theme, const SubscribeCallback &cb) {
    if (cb == nullptr) {
        return;
    }

    if (HasSubscriber(theme) == false) {
        pubsub_[theme] = {};
    }
    pubsub_[theme].emplace_back(cb);
}

bool CoBridge::HasSubscriber(const std::string &theme) const { return pubsub_.count(theme) > 0; }
