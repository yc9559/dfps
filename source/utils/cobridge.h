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

#pragma once

#include "singleton.h"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class CoBridge : public Singleton<CoBridge> {
public:
    using SubscribeCallback = std::function<void(const void *)>;

    CoBridge();
    void Publish(const std::string &theme, const void *data) const;
    void Subscribe(const std::string &theme, const SubscribeCallback &cb);

    template <typename T>
    static const T &Get(const void *data) {
        return *reinterpret_cast<const T *>(data);
    }

private:
    std::unordered_map<std::string, std::vector<SubscribeCallback>> pubsub_;
};
