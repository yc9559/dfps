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

#include <string>
#include <vector>

int64_t GetNowTs(void);
int64_t MsToUs(int64_t ms);

bool IsSpace(const char c);
bool IsDigit(const char c);
bool ParseInt(const char *s, int32_t *val);

void GetUnsignedIntFromFile(const std::string &path, std::vector<int> *numbers);

void SetSelfThreadName(const std::string &name);
