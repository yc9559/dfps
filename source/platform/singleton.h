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

#include <memory>
#include <mutex>

template <typename T>
class Singleton {
public:
    Singleton() = default;
    Singleton(const Singleton &) = delete;
    Singleton &operator=(const Singleton &) = delete;

    static T *GetInstance(void) {
        std::call_once(once_, []() { instance_ = new T(); });
        return instance_;
    }

private:
    static T *instance_;
    static std::once_flag once_;
};

template <typename T>
T *Singleton<T>::instance_ = nullptr;

template <typename T>
std::once_flag Singleton<T>::once_;
