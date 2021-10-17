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
#include <cstring>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include "utils/misc.h"

std::string GetTopAppNameDumpsys(void) {
    constexpr char DUMPSYS_CMD[] = "/system/bin/dumpsys activity o";
    constexpr char TA_KEYWORD[] = " (top-activity)";
    constexpr int DUMPSYS_EXEC_WAIT_US = MsToUs(80);
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

std::string GetTombstone(int pid) {
    constexpr char TOMBSTONE_DIR[] = "/data/tombstones";
    constexpr char TOMBSTONE_STOP_KEYWORD[] = "stack";
    constexpr int TOMBSTONE_HEAD_LINES = 8;
    constexpr int TOMBSTONE_MAX_LINES = 32;

    if (access(TOMBSTONE_DIR, F_OK) != 0) {
        return {};
    }

    char path[PATH_MAX];
    std::string latest;
    int64_t latestTs = 0;
    struct stat st;

    // get the latest tombstone
    DIR *tsfp = opendir(TOMBSTONE_DIR);
    struct dirent *dp;
    while ((dp = readdir(tsfp))) {
        // skip . and ..
        if (dp->d_name[0] == '.') {
            continue;
        }
        snprintf(path, sizeof(path), "%s/%s", TOMBSTONE_DIR, dp->d_name);
        if (stat(path, &st) == 0) {
            int64_t ts = st.st_mtim.tv_sec * 1e9 + st.st_mtim.tv_nsec;
            if (ts > latestTs) {
                latest = path;
                latestTs = ts;
            }
        }
    }
    closedir(tsfp);

    if (latestTs == 0) {
        return {};
    }

    FILE *fp = fopen(latest.c_str(), "re");
    if (fp == NULL) {
        return {};
    }

    char buf[512];
    bool isThisFile = false;
    for (int i = 0; i < TOMBSTONE_HEAD_LINES; ++i) {
        if (feof(fp))
            break;
        // pid: 17096, tid: 17098, name: unwind  >>> ./unwind <<<
        int owner_pid = 0;
        fgets(buf, sizeof(buf), fp);
        sscanf(buf, "%*s %d", &owner_pid);
        if (owner_pid == pid) {
            isThisFile = true;
            break;
        }
    }
    if (isThisFile == false) {
        fclose(fp);
        return {};
    }

    // skip the first line of *****
    rewind(fp);

    std::string bt;
    fgets(buf, sizeof(buf), fp);
    for (int i = 0; i < TOMBSTONE_MAX_LINES; ++i) {
        if (feof(fp)) {
            break;
        }
        // print backtrace only
        fgets(buf, sizeof(buf), fp);
        if (strncmp(buf, TOMBSTONE_STOP_KEYWORD, strlen(TOMBSTONE_STOP_KEYWORD)) == 0) {
            break;
        }
        bt += buf;
    }

    fclose(fp);
    return bt;
}

void SyncCallPutRefreshRate(const char *key, const char *hz) {
    pid_t pid = fork();
    if (pid == -1) {
        return;
    } else if (pid == 0) {
        execl("/system/bin/cmd", "/system/bin/cmd", "settings", "put", "system", key, hz, nullptr);
        exit(-1);
    } else {
        waitpid(pid, nullptr, 0);
    }
}

void SyncCallSurfaceflingerBackdoor(const char *code, const char *hz) {
    pid_t pid = fork();
    if (pid == -1) {
        return;
    } else if (pid == 0) {
        execl("/system/bin/service", "/system/bin/service", "call", "SurfaceFlinger", code, "i32", hz, nullptr);
        exit(-1);
    } else {
        waitpid(pid, nullptr, 0);
    }
}

void SysPeakRefreshRate(const std::string &hz, bool force) {
    SyncCallPutRefreshRate("peak_refresh_rate", hz.c_str());
    SyncCallPutRefreshRate("min_refresh_rate", hz.c_str());
}

void SysSurfaceflingerBackdoor(const std::string &idx, bool force) {
    // >= Android 10
    // 1035 -1/0/1/2: setActiveConfig
    SyncCallSurfaceflingerBackdoor("1035", idx.c_str());

    if (force) {
        // >= Android 11
        // 1036 1: Frame rate flexibility token acquired. count=1
        // 1036 0: Frame rate flexibility token released. count=0
        // service call SurfaceFlinger 1035 i32 -1 -- okay
        // service call SurfaceFlinger 1036 i32 1
        // service call SurfaceFlinger 1035 i32 2 -- not working
        SyncCallSurfaceflingerBackdoor("1036", "1");
        SyncCallSurfaceflingerBackdoor("1035", "-1");
        SyncCallSurfaceflingerBackdoor("1036", "0");
        SyncCallSurfaceflingerBackdoor("1035", idx.c_str());
    }
}
