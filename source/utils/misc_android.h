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

int GetOSVersion(void);
std::string GetTopAppNameDumpsys(void);
std::string GetHomePackageName(void);
std::string GetTombstone(int pid);

void SysPeakRefreshRate(const std::string &hz, bool force);
void SysSurfaceflingerBackdoor(const std::string &idx, bool force);
