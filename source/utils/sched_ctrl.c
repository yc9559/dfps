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

#include "sched_ctrl.h"
#include <sys/capability.h>
#include <sys/resource.h>

#define MIN_RT_PRIO (1)
#define MAX_RT_PRIO (99)
#define MIN_NORM_PRIO (100)
#define MAX_NORM_PRIO (139)
#define NORMAL_PRIO (120)
#define BATCH_PRIO (140)
#define IDLE_PRIO (141)

int SchedCtrlSetAffinity(int tid, const cpu_set_t *mask) { return sched_setaffinity(tid, sizeof(cpu_set_t), mask); }

int SchedCtrlGetStaticPrio(int tid) {
    int prio = MAX_NORM_PRIO - 1;

    switch (sched_getscheduler(tid)) {
        case SCHED_NORMAL: {
            int nice = getpriority(PRIO_PROCESS, tid);
            prio = NORMAL_PRIO + nice;
            break;
        }
        case SCHED_BATCH:
            prio = BATCH_PRIO;
            break;
        case SCHED_IDLE:
            prio = IDLE_PRIO;
            break;
        case SCHED_FIFO:
        case SCHED_RR:
        case SCHED_DEADLINE: {
            struct sched_param param;
            sched_getparam(tid, &param);
            prio = MAX_RT_PRIO - param.sched_priority;
            break;
        }
        default:
            break;
    }

    return prio;
}

int SchedCtrlSetSchedClass(int tid, int policy, int prio) {
    struct sched_param param;
    param.sched_priority = prio;
    return sched_setscheduler(tid, policy, &param);
}

static int SetNormStaticPrio(int tid, int prio) {
    int ret = SchedCtrlSetSchedClass(tid, SCHED_NORMAL, 0);
    if (ret) {
        return ret;
    }
    return setpriority(PRIO_PROCESS, tid, prio - NORMAL_PRIO);
}

int SchedCtrlSetStaticPrio(int tid, int prio, bool resetOnFork) {
    if (MIN_RT_PRIO <= prio && prio <= MAX_RT_PRIO) {
        int policy = SCHED_FIFO;
        if (resetOnFork) {
            policy |= SCHED_RESET_ON_FORK;
        }
        return SchedCtrlSetSchedClass(tid, policy, MAX_RT_PRIO - prio);
    }

    if (MIN_NORM_PRIO <= prio && prio <= MAX_NORM_PRIO) {
        return SetNormStaticPrio(tid, prio);
    }

    return -1;
}

int SchedCtrlRemoveNiceCap(int tid) {
    struct __user_cap_header_struct header = {0};
    struct __user_cap_data_struct data[CAP_TO_INDEX(CAP_LAST_CAP) + 1] = {0};

    header.version = _LINUX_CAPABILITY_VERSION_3;
    header.pid = tid;
    if (capget(&header, data)) {
        return -1;
    }

    header.version = _LINUX_CAPABILITY_VERSION_3;
    header.pid = tid;
    int idx = CAP_TO_INDEX(CAP_SYS_NICE);
    unsigned int mask = CAP_TO_MASK(CAP_SYS_NICE);
    data[idx].effective &= ~mask;
    data[idx].permitted &= ~mask;
    data[idx].inheritable &= ~mask;
    return capset(&header, data);
}
