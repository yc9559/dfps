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

#include <string>
#include <vector>

int64_t GetNowTs(void);
void Sleep(int64_t us);

constexpr int64_t MsToUs(int64_t ms) { return ms * 1000; }
constexpr int64_t SToUs(double second) { return second * 1000 * 1000; }
constexpr int64_t UsToMs(int64_t us) { return us / 1000; }
constexpr int64_t UsToS(int64_t us) { return us / 1000 / 1000; }

bool IsSpace(const char c);
bool IsDigit(const char c);
bool ParseInt(const char *s, int32_t *val);
std::string_view RefKey(const std::string &fullname);

void InitArgv(int argc, char **argv);
void SetSelfName(const std::string_view &name);
void SetSelfThreadName(const std::string_view &name);

int ReadFile(const std::string_view &path, std::string *content, size_t maxLen = 0);
int WriteFile(const std::string_view &path, const std::string_view &s);
int WriteFile(const std::string_view &path, int s);
int WriteFile(int fd, const std::string_view &s);
int WriteFile(int fd, int s);
int GetFdForWrite(const std::string_view &path);
int GetFdForWriteExcluded(const std::string_view &path);

int ExecCmd(int *fd, const char **argv);
template <class... Args>
int ExecCmd(int *pipefd, Args &&...args) {
    const char *argv[] = {args..., nullptr};
    return ExecCmd(pipefd, argv);
}

int ExecCmdSync(std::string *content, const char **argv);
template <class... Args>
int ExecCmdSync(std::string *content, Args &&...args) {
    const char *argv[] = {args..., nullptr};
    return ExecCmdSync(content, argv);
}

template <typename T>
T Cap(T val, T floor, T ceil) {
    return std::max(floor, std::min(ceil, val));
}

template <typename T>
struct TypeParseTraits;

#define REGISTER_PARSE_TYPE(X)                                                                                         \
    template <>                                                                                                        \
    struct TypeParseTraits<X> {                                                                                        \
        static constexpr char name[] = #X;                                                                             \
    };
