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
    std::mutex mut_;
    std::condition_variable cv_;
    std::thread th_;
};
