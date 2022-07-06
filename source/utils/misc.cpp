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

#include "misc.h"
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <scn/scn.h>
#include <signal.h>
#include <spdlog/spdlog.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int64_t GetNowTs(void) {
    auto ns = std::chrono::steady_clock::time_point::clock().now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(ns).count();
}

void Sleep(int64_t us) { usleep(us); }

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

std::string_view RefKey(const std::string &fullname) {
    std::string_view view = fullname;
    auto pos = view.find_first_of('.');
    return (pos < view.length()) ? view.substr(pos + 1) : std::string_view();
}

static char *argv0;
static size_t nameLen;

void InitArgv(int argc, char **argv) {
    argv0 = argv[0];
    nameLen = (argv[argc - 1] - argv[0]) + strlen(argv[argc - 1]) + 1;
}

void SetSelfName(const std::string_view &name) {
    memset(argv0, 0, nameLen);
    strlcpy(argv0, name.data(), nameLen);
    SetSelfThreadName(name);
}

void SetSelfThreadName(const std::string_view &name) { prctl(PR_SET_NAME, name.data()); }

constexpr int READER_FD_FLAGS = O_RDONLY | O_NONBLOCK | O_CLOEXEC;
constexpr int WRITER_FD_FLAGS = O_WRONLY | O_NONBLOCK | O_CLOEXEC;
constexpr int WRITER_PERM = 0666;
constexpr int WRITER_EXCLUDED_PERM = 0444;

int ReadFile(const std::string_view &path, std::string *content, size_t maxLen) {
    constexpr size_t READ_DEFAULT_SIZE = 4096;
    if (content == nullptr) {
        return -1;
    }
    content->clear();

    int fd = open(path.data(), READER_FD_FLAGS);
    if (fd <= 0) {
        return -1;
    }

    if (maxLen == 0) {
        maxLen = READ_DEFAULT_SIZE;
    }
    content->resize(maxLen);
    maxLen--; // reserve for EOF

    // < 4096 bytes
    if (maxLen < READ_DEFAULT_SIZE) {
        auto len = read(fd, content->data(), maxLen);
        if (len > 0) {
            content->data()[len] = '\0';
            content->resize(len);
        } else {
            content->data()[0] = '\0';
            content->clear();
        }
        close(fd);
        return len;
    }

    // >= 4096 bytes
    int len = 0;
    int l;
    while ((l = read(fd, content->data() + len, maxLen - len)) > 0) {
        len += l;
    }
    if (len > 0) {
        content->data()[len] = '\0';
        content->resize(len);
    } else {
        content->data()[0] = '\0';
        content->clear();
    }
    close(fd);
    return len;
}

int WriteFile(const std::string_view &path, const std::string_view &s) {
    auto fd = GetFdForWrite(path);
    if (fd > 0) {
        auto ret = WriteFile(fd, s);
        close(fd);
        return ret;
    }
    return -1;
}

int WriteFile(const std::string_view &path, int s) {
    auto fd = GetFdForWrite(path);
    if (fd > 0) {
        auto ret = WriteFile(fd, s);
        close(fd);
        return ret;
    }
    return -1;
}

int WriteFile(int fd, const std::string_view &s) {
    if (fd <= 0) {
        return -1;
    }
    lseek(fd, 0, SEEK_SET);
    return write(fd, s.data(), s.length());
}

int WriteFile(int fd, int s) {
    if (fd <= 0) {
        return -1;
    }
    lseek(fd, 0, SEEK_SET);
    return write(fd, &s, sizeof(s));
}

int GetFdForWrite(const std::string_view &path) {
    auto fd = open(path.data(), WRITER_FD_FLAGS);
    if (fd <= 0) {
        chmod(path.data(), WRITER_PERM);
        fd = open(path.data(), WRITER_FD_FLAGS);
    }
    return fd;
}

int GetFdForWriteExcluded(const std::string_view &path) {
    auto fd = open(path.data(), WRITER_FD_FLAGS);
    if (fd <= 0) {
        chmod(path.data(), WRITER_PERM);
        fd = open(path.data(), WRITER_FD_FLAGS);
    }
    chmod(path.data(), WRITER_EXCLUDED_PERM);
    chown(path.data(), 0, 0); // root
    return fd;
}

int ExecCmd(int *fd, const char **argv) {
    int pipefd[] = {-1, -1};
    auto &parentFd = pipefd[0];
    auto &childFd = pipefd[1];

    if (fd) {
        if (pipe2(pipefd, O_CLOEXEC) == -1) {
            return -1;
        }
    }

    int pid = fork();
    if (pid < 0) {
        close(parentFd);
        close(childFd);
        return -1;
    } else if (pid) {
        if (fd) {
            *fd = parentFd;
            close(childFd);
        }
        return pid;
    }

    // Unblock all signals
    sigset_t s;
    sigfillset(&s);
    pthread_sigmask(SIG_UNBLOCK, &s, nullptr);

    if (childFd >= 0) {
        dup2(childFd, STDOUT_FILENO);
        dup2(childFd, STDERR_FILENO);
        close(childFd);
    }

    execv(argv[0], (char *const *)argv);
    exit(-1);
}

int ExecCmdSync(std::string *content, const char **argv) {
    int fd = -1;
    int pid = ExecCmd(content ? &fd : nullptr, argv);
    if (pid < 0) {
        return -1;
    }

    int status;
    waitpid(pid, &status, 0);
    status = WEXITSTATUS(status);

    if (content) {
        constexpr size_t READ_BATCH_SIZE = 4096;
        int len = 0;
        int l = 0;
        content->clear();
        do {
            len += l;
            content->resize(len + READ_BATCH_SIZE);
        } while ((l = read(fd, content->data() + len, READ_BATCH_SIZE)) > 0);
        close(fd);

        if (len > 0) {
            content->resize(len + 1);
            content->back() = '\0';
        } else {
            content->clear();
        }
    }

    return status;
}
