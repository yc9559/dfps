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

#include "misc.h"
#include <chrono>

int64_t GetNowTs(void) {
    auto ns = std::chrono::steady_clock::time_point::clock().now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(ns).count();
}
