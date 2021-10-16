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
#include <cstring>
#include <sys/prctl.h>

int64_t GetNowTs(void) {
    auto ns = std::chrono::steady_clock::time_point::clock().now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(ns).count();
}

bool IsSpace(const char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r'; }

bool IsDigit(const char c) { return c >= '0' && c <= '9'; }

bool ParseInt(const char *s, int32_t *val) {
    int len = 0;
    int sign = 1;
    *val = 0;

    // locate beginning of digit
    for (const char *c = s; *c; ++c) {
        if (*c == '-') {
            sign = -1;
        }
        if (IsDigit(*c)) {
            s = c;
            break;
        }
    }

    // get the length of digits
    while (*s == '0' && IsDigit(*(s + 1))) {
        ++s;
    }
    for (const char *c = s; *c; ++c) {
        if (!IsDigit(*c)) {
            break;
        }
        ++len;
    }

    // assume number in the range of int32_t
    switch (len) {
        case 10:
            *val += (s[len - 10] - '0') * 1000000000;
        case 9:
            *val += (s[len - 9] - '0') * 100000000;
        case 8:
            *val += (s[len - 8] - '0') * 10000000;
        case 7:
            *val += (s[len - 7] - '0') * 1000000;
        case 6:
            *val += (s[len - 6] - '0') * 100000;
        case 5:
            *val += (s[len - 5] - '0') * 10000;
        case 4:
            *val += (s[len - 4] - '0') * 1000;
        case 3:
            *val += (s[len - 3] - '0') * 100;
        case 2:
            *val += (s[len - 2] - '0') * 10;
        case 1:
            *val += (s[len - 1] - '0');
            *val *= sign;
            return true;
        default:
            break;
    }
    return false;
}

ssize_t ReadFile(const char *path, char *buf, size_t max_len) {
    if (buf == NULL || max_len == 0) {
        return -1;
    }

    FILE *fp = fopen(path, "re");
    if (fp == NULL) {
        return -1;
    }

    size_t len = fread(buf, sizeof(char), max_len, fp);
    buf[std::min(len, max_len - 1)] = '\0';
    fclose(fp);
    return len;
}

void GetUnsignedIntFromFile(const std::string &path, std::vector<int> *numbers) {
    constexpr std::size_t FILE_BUF_SIZE = 32 * 1024;
    constexpr char DELIMITER[] = " \n\t\r";
    numbers->clear();

    char buf[FILE_BUF_SIZE];
    ssize_t len = ReadFile(path.c_str(), buf, sizeof(buf));
    if (len > 0) {
        char *pos = NULL;
        char *num = strtok_r(buf, DELIMITER, &pos);
        while (num) {
            int tid;
            if (ParseInt(num, &tid) == false) {
                break;
            }
            numbers->emplace_back(tid);
            num = strtok_r(NULL, DELIMITER, &pos);
        }
    }
}

void SetSelfThreadName(const std::string &name) { prctl(PR_SET_NAME, name.c_str()); }
