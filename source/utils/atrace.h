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

#include <stdbool.h>
#include <stdlib.h>

__BEGIN_DECLS

int AtraceInit(void);
bool AtraceIsEnable(void);
void AtraceToggle(bool enable);
void AtraceCntWrite(const char *tag, long long cnt);
void AtraceScopeBegin(const char *tag);
void AtraceScopeEnd(void);

#define ATRACE_NAME(name)                                                                                              \
    {                                                                                                                  \
        static unsigned long long cnt = 0;                                                                             \
        if (AtraceIsEnable()) {                                                                                        \
            AtraceCntWrite(name, (cnt++) & 0x1);                                                                       \
        }                                                                                                              \
    }

#define ATRACE_CALL() ATRACE_NAME(__func__)

__END_DECLS

#ifdef __cplusplus

#include <string_view>

struct AtraceScopeHelper {
    AtraceScopeHelper(const std::string_view &tag) { AtraceScopeBegin(tag.data()); }
    ~AtraceScopeHelper() { AtraceScopeEnd(); }
};

#define ATRACE_SCOPE_FUNC() AtraceScopeHelper __ash(__PRETTY_FUNCTION__)
#define ATRACE_SCOPE(name) AtraceScopeHelper __ash(#name)

#endif
