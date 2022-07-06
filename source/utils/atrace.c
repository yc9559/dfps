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

#include "atrace.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

static bool atrace_enable;
static int tracemark_fd;
static int self_pid;

int AtraceInit(void) {
    atrace_enable = false;
    self_pid = getpid();

    tracemark_fd = open("/sys/kernel/debug/tracing/trace_marker", O_WRONLY | O_CLOEXEC);
    if (tracemark_fd > 0) {
        return 0;
    }

    tracemark_fd = open("/sys/kernel/tracing/trace_marker", O_WRONLY | O_CLOEXEC);
    if (tracemark_fd > 0) {
        return 0;
    }
    return -1;
}

bool AtraceIsEnable(void) { return atrace_enable && tracemark_fd > 0; }

void AtraceToggle(bool enable) { atrace_enable = enable; }

void AtraceCntWrite(const char *tag, long long cnt) {
    if (AtraceIsEnable()) {
        char buf[128];
        int len = snprintf(buf, sizeof(buf), "C|%d|%s|%lld", self_pid, tag, cnt);
        write(tracemark_fd, buf, len);
    }
}

void AtraceScopeBegin(const char *tag) {
    if (AtraceIsEnable()) {
        char buf[128];
        int len = snprintf(buf, sizeof(buf), "B|%d|%s", self_pid, tag);
        write(tracemark_fd, buf, len);
    }
}

void AtraceScopeEnd(void) {
    if (AtraceIsEnable()) {
        char buf[128];
        int len = snprintf(buf, sizeof(buf), "E|%d", self_pid);
        write(tracemark_fd, buf, len);
    }
}
