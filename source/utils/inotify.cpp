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

#include "inotify.h"
#include "fmt_exception.h"
#include <fcntl.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <unistd.h>

constexpr int WAIT_MOVE_US = 500 * 1000;
constexpr int RECREATE_DEFAULT_PERM = 666;
constexpr std::size_t EVT_BUF_SIZE = 65536;

Inotify::Inotify() : buf_(EVT_BUF_SIZE) {
    fd_ = inotify_init();
    if (fd_ < 0) {
        throw FmtException("Cannot make inotifier handle");
    }
}

Inotify::~Inotify() {
    for (const auto &[wd, item] : items_) {
        inotify_rm_watch(fd_, wd);
    }
    close(fd_);
}

void Inotify::Add(const std::string &path, int mask, Notifier notifier) {
    mask |= DELETE_SELF | MOVE_SELF;
    auto wd = inotify_add_watch(fd_, path.c_str(), mask);
    if (wd < 0) {
        throw FmtException("Cannot watch file '{}'", path);
    }

    WatchItem item;
    item.path = path;
    item.mask = mask;
    item.notifier = notifier;
    items_[wd] = item;
}

void Inotify::WaitAndHandle(void) {
    auto len = read(fd_, buf_.data(), buf_.size());
    ssize_t off = 0;
    while (off < len) {
        auto evt = reinterpret_cast<struct inotify_event *>(buf_.data() + off);
        off += sizeof(struct inotify_event) + evt->len;
        auto it = items_.find(evt->wd);
        if (it == items_.end()) {
            continue;
        }
        const auto &item = it->second;
        // re-establish watching after deleting
        if (evt->mask & (IGNORED | DELETE_SELF | MOVE_SELF)) {
            const auto &path = item.path.c_str();
            if (access(path, F_OK) != 0) {
                usleep(WAIT_MOVE_US);
                if (access(path, F_OK) != 0) {
                    int fd = creat(path, RECREATE_DEFAULT_PERM);
                    if (fd > 0) {
                        close(fd);
                    }
                }
            }
            auto wd = inotify_add_watch(fd_, path, item.mask);
            items_[wd] = item;
            items_.erase(evt->wd);
        }
        if (item.notifier && (evt->mask & item.mask)) {
            item.notifier(item.path, evt->mask);
        }
    }
}
