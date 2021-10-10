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

#include "cobridge.h"

DECLARE_SINGLETON(CoBridge)

CoBridge::CoBridge() {}

void CoBridge::Publish(const std::string &theme, void *data) const {
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

    if (pubsub_.count(theme) == 0) {
        pubsub_[theme] = {};
    }
    pubsub_[theme].emplace_back(cb);
}
