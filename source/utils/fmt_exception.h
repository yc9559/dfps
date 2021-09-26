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

#include <exception>
#include <spdlog/fmt/fmt.h>
#include <string>

class FmtException : public std::exception {
public:
    template <typename... Args>
    FmtException(fmt::format_string<Args...> fmt, Args &&...args)
        : msg_(fmt::format(fmt, std::forward<Args>(args)...)) {}

    const char *what(void) const noexcept override { return msg_.c_str(); }

protected:
    std::string msg_;
};
