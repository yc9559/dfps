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

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class Inotify {
public:
    using Notifier = std::function<void(const std::string &, int)>;
    enum {
        ACCESS = 0x00000001,
        MODIFY = 0x00000002,
        ATTRIB = 0x00000004,
        CLOSE_WRITE = 0x00000008,
        CLOSE_NOWRITE = 0x00000010,
        OPEN = 0x00000020,
        MOVED_FROM = 0x00000040,
        MOVED_TO = 0x00000080,
        CREATE = 0x00000100,
        DELETE = 0x00000200,
        DELETE_SELF = 0x00000400,
        MOVE_SELF = 0x00000800,
        UNMOUNT = 0x00002000,
        Q_OVERFLOW = 0x00004000,
        IGNORED = 0x00008000,
        CLOSE = (CLOSE_WRITE | CLOSE_NOWRITE),
        MOVE = (MOVED_FROM | MOVED_TO),
        ONLYDIR = 0x01000000,
        DONT_FOLLOW = 0x02000000,
        EXCL_UNLINK = 0x04000000,
        MASK_CREATE = 0x10000000,
        MASK_ADD = 0x20000000,
        ISDIR = 0x40000000,
        ONESHOT = 0x80000000
    };

    Inotify();
    ~Inotify();

    void Add(const std::string &path, int mask, Notifier notifier);
    void WaitAndHandle(void);

private:
    struct WatchItem {
        Notifier notifier;
        std::string path;
        int mask;
    };

    int fd_;
    std::unordered_map<int, WatchItem> items_;
    std::vector<uint8_t> buf_;
};
