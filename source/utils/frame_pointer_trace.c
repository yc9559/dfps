// https://chromium.googlesource.com/chromium/src/base/+/master/debug/stack_trace.cc
//
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

#if defined(__arm__) && defined(__GNUC__) && !defined(__clang__)
// GCC and LLVM generate slightly different frames on ARM, see
// https://llvm.org/bugs/show_bug.cgi?id=18505 - LLVM generates
// x86-compatible frame, while GCC needs adjustment.
const size_t kStackFrameAdjustment = sizeof(uintptr_t);
#else
const size_t kStackFrameAdjustment = 0;
#endif

uintptr_t get_nextstackframe(uintptr_t fp) {
    const uintptr_t *fp_addr = (const uintptr_t *)(fp);
    return fp_addr[0] - kStackFrameAdjustment;
}

uintptr_t get_stackframepc(uintptr_t fp) {
    const uintptr_t *fp_addr = (const uintptr_t *)(fp);
    return fp_addr[1];
}

bool is_stackframe_valid(uintptr_t fp, uintptr_t prev_fp, uintptr_t stack_end) {
    // With the stack growing downwards, older stack frame must be
    // at a greater address that the current one.
    if (fp <= prev_fp)
        return false;
    // Assume huge stack frames are bogus.
    if (fp - prev_fp > 100000)
        return false;
    // Check alignment.
    if (fp & (sizeof(uintptr_t) - 1))
        return false;
    if (stack_end) {
        // Both fp[0] and fp[1] must be within the stack.
        if (fp > stack_end - 2 * sizeof(uintptr_t))
            return false;
        // Additional check to filter out false positives.
        if (get_stackframepc(fp) < 32768)
            return false;
    }
    return true;
}

uintptr_t get_stackend() {
    // Bionic reads proc/maps on every call to pthread_getattr_np() when called
    // from the main thread. So we need to cache end of stack in that case to get
    // acceptable performance.
    // For all other threads pthread_getattr_np() is fast enough as it just reads
    // values from its pthread_t argument.
    static uintptr_t main_stack_end = 0;
    bool is_main_thread = getpid() == gettid();
    if (is_main_thread && main_stack_end) {
        return main_stack_end;
    }
    uintptr_t stack_begin = 0;
    size_t stack_size = 0;
    pthread_attr_t attributes;
    int error = pthread_getattr_np(pthread_self(), &attributes);
    if (!error) {
        error = pthread_attr_getstack(&attributes, (void **)(&stack_begin), &stack_size);
        pthread_attr_destroy(&attributes);
    }
    uintptr_t stack_end = stack_begin + stack_size;
    if (is_main_thread) {
        main_stack_end = stack_end;
    }
    return stack_end; // 0 in case of error
}

size_t trace_stackframepointers(void **out_trace, size_t max_depth, size_t skip_initial) {
    // Usage of __builtin_frame_address() enables frame pointers in this
    // function even if they are not enabled globally. So 'fp' will always
    // be valid.
    uintptr_t fp = (uintptr_t)(__builtin_frame_address(0)) - kStackFrameAdjustment;
    uintptr_t stack_end = get_stackend();
    size_t depth = 0;
    while (depth < max_depth) {
        if (skip_initial != 0) {
            skip_initial--;
        } else {
            out_trace[depth++] = (void *)(get_stackframepc(fp));
        }
        uintptr_t next_fp = get_nextstackframe(fp);
        if (is_stackframe_valid(next_fp, fp, stack_end)) {
            fp = next_fp;
            continue;
        }
        // Failed to find next frame.
        break;
    }
    return depth;
}
