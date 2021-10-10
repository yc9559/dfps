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

#include "misc_android.h"
#include <unistd.h>
#include <stdlib.h>
#include <vector>
#include <cstring>

std::string GetTopAppNameDumpsys(void)
{
    constexpr char DUMPSYS_CMD[] = "/system/bin/dumpsys activity o";
    constexpr char TA_KEYWORD[] = "(top-activity)";
    constexpr int DUMPSYS_EXEC_WAIT_US = 80 * 1000;
    constexpr std::size_t BUF_SIZE = 256 * 1024;

    FILE *fs = popen(DUMPSYS_CMD, "r");
    usleep(DUMPSYS_EXEC_WAIT_US);
    if (fs == NULL) {
        return {};
    }

    std::vector<char> buf(BUF_SIZE);
    size_t len = fread(buf.data(), sizeof(char), buf.size(), fs);
    len = std::min(len, buf.size() - 1);
    buf[len] = '\0';
    pclose(fs);

    // venus:/ # dumpsys activity o | grep top-activity
    //     Proc # 3: fg     T/A/TOP  LCM  t: 0 30265:com.tencent.mm:toolsmp/u0a272 (top-activity)
    // venus:/ # dumpsys activity o | grep top-activity
    //     Proc # 0: fg     T/A/TOP  LCM  t: 0 2849:com.miui.home/u0a91 (top-activity)
    // osborn:/ # dumpsys activity o | grep top-activity
    //     Proc # 1: fore  T/A/TOP  trm: 0 2899:com.teslacoilsw.launcher/u0a178 (top-activity)

    char *topappName = strstr(buf.data(), TA_KEYWORD);
    if (topappName == nullptr) {
        return {};
    }
    topappName--;
    while (topappName > buf.data() && *topappName != ' ') {
        topappName--;
    }
    while (topappName < buf.data() + len && *topappName != ':') {
        topappName++;
    }
    topappName++;

    char *end = std::min(strchr(topappName, '/'), strchr(topappName, ':'));
    if (end == nullptr) {
        return {};
    }
    *end = '\0';
    return topappName;
}
