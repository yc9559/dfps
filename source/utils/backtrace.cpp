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

#include "backtrace.h"
#include "frame_pointer_trace.h"
#include <cxxabi.h>
#include <dlfcn.h>
#include <signal.h>
#include <spdlog/spdlog.h>
#include <string>
#include <unistd.h>
#include <unwind.h>

constexpr size_t MAX_FRAMES = 64;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"

struct UnwindClang {
    size_t frame_num;
    ucontext_t *uc;
    uintptr_t prev_pc;
    uintptr_t prev_sp;
    uintptr_t sig_pc;
    uintptr_t sig_lr;
    int found_sig_pc;
};

#pragma clang diagnostic pop

std::string Demangle(const char *mangled) {
    std::string str = mangled;
    int status = 0;
    char *demangled = __cxxabiv1::__cxa_demangle(mangled, 0, 0, &status);
    if (demangled) {
        str = demangled;
        free(demangled);
    }
    return str;
}

int RecordFrameClang(UnwindClang *self, uintptr_t pc) {
    Dl_info info;
    uintptr_t addr;
    std::string symbol;

    // append line for current frame
    if (0 == dladdr((void *)pc, &info) || (uintptr_t)info.dli_fbase > pc) {
        addr = pc;
        symbol = "?";
    } else {
        addr = pc - (uintptr_t)info.dli_fbase;
        if (NULL == info.dli_fname || '\0' == info.dli_fname[0]) {
            symbol = fmt::format("<anonymous:{:#x}>", (uintptr_t)info.dli_fbase);
        } else {
            if (NULL == info.dli_sname || '\0' == info.dli_sname[0]) {
                symbol = fmt::format("{} (?)", info.dli_fname);
            } else {
                if (0 == (uintptr_t)info.dli_saddr || (uintptr_t)info.dli_saddr > pc) {
                    symbol = fmt::format("{} ({})", info.dli_fname, Demangle(info.dli_sname));
                } else {
                    symbol = fmt::format("{} ({}+{:#x})", info.dli_fname, Demangle(info.dli_sname),
                                         pc - (uintptr_t)info.dli_saddr);
                }
            }
        }
    }
    SPDLOG_ERROR("#{:02}: {:#016x} {}", self->frame_num, addr, symbol);

    // check unwinding limit
    self->frame_num++;
    if (self->frame_num >= MAX_FRAMES) {
        return 1;
    }

    return 0;
}

static _Unwind_Reason_Code UnwindClangCallback(struct _Unwind_Context *unw_ctx, void *arg) {
    UnwindClang *self = (UnwindClang *)arg;
    uintptr_t pc = _Unwind_GetIP(unw_ctx);
    uintptr_t sp = _Unwind_GetCFA(unw_ctx);

    if (0 == self->found_sig_pc) {
        if ((self->sig_pc >= sizeof(uintptr_t) && pc <= self->sig_pc + sizeof(uintptr_t) &&
             pc >= self->sig_pc - sizeof(uintptr_t)) ||
            (self->sig_lr >= sizeof(uintptr_t) && pc <= self->sig_lr + sizeof(uintptr_t) &&
             pc >= self->sig_lr - sizeof(uintptr_t))) {
            self->found_sig_pc = 1;
        } else {
            return _URC_NO_REASON; // skip and continue
        }
    }

    if (self->frame_num > 0 && pc == self->prev_pc && sp == self->prev_sp) {
        return _URC_END_OF_STACK; // stop
    }

    if (0 != RecordFrameClang(self, pc)) {
        return _URC_END_OF_STACK; // stop
    }

    self->prev_pc = pc;
    self->prev_sp = sp;

    return _URC_NO_REASON; // continue
}

void DumpBacktraceClang(ucontext_t *uc) {
    UnwindClang self;
    memset(&self, 0, sizeof(UnwindClang));
    self.uc = uc;

    // Trying to get LR for x86 and x86_64 on local unwind is usually
    // leads to access unmapped memory, which will crash the process immediately.
#if defined(__arm__)
    self.sig_pc = (uintptr_t)uc->uc_mcontext.arm_pc;
    self.sig_lr = (uintptr_t)uc->uc_mcontext.arm_lr;
#elif defined(__aarch64__)
    self.sig_pc = (uintptr_t)uc->uc_mcontext.pc;
    self.sig_lr = (uintptr_t)uc->uc_mcontext.regs[30];
#elif defined(__i386__)
    self.sig_pc = (uintptr_t)uc->uc_mcontext.gregs[REG_EIP];
#elif defined(__x86_64__)
    self.sig_pc = (uintptr_t)uc->uc_mcontext.gregs[REG_RIP];
#endif

    SPDLOG_ERROR("Unwind backtrace:");
    _Unwind_Backtrace(UnwindClangCallback, &self);
}

void DumpBacktraceFramePointer(ucontext_t *uc) {
    void *pointers[MAX_FRAMES];
    auto len = trace_stackframepointers(pointers, MAX_FRAMES, 0);

    SPDLOG_ERROR("Framepointer backtrace:");
    for (size_t i = 0; i < len; ++i) {
        SPDLOG_ERROR("#{:02}: {:#016x}", i, (uintptr_t)pointers[i]);
    }
}

void DumpBacktrace(ucontext_t *uc) {
    DumpBacktraceClang(uc);
    // DumpBacktraceFramePointer(uc);
}

void CrashSignalHandler(int sig, siginfo_t *si, void *uc) {
    SPDLOG_ERROR("Receive signal {}, terminated", sig);
    DumpBacktrace(reinterpret_cast<ucontext_t *>(uc));
    exit(EXIT_FAILURE);
}

void SetDumpBacktraceAsCrashHandler(void) {
    int crashSignals[] = {SIGILL, SIGABRT, SIGFPE, SIGSEGV, SIGBUS, SIGSYS};

    struct sigaction act;
    memset(&act, 0, sizeof(act));
    sigfillset(&act.sa_mask);
    act.sa_sigaction = CrashSignalHandler;
    act.sa_flags = SA_RESTART | SA_SIGINFO;

    for (int i = 0; i < sizeof(crashSignals) / sizeof(int); ++i) {
        sigaction(crashSignals[i], &act, nullptr);
    }
}
