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

#include <sched.h>
#include <stdbool.h>
#include <stdlib.h>

__BEGIN_DECLS

int SchedCtrlSetAffinity(int tid, const cpu_set_t *mask);
int SchedCtrlGetStaticPrio(int tid);
int SchedCtrlSetStaticPrio(int tid, int prio, bool resetOnFork);
int SchedCtrlSetSchedClass(int tid, int policy, int prio);
int SchedCtrlRemoveNiceCap(int tid);

__END_DECLS
