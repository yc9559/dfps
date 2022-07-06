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

#include "input_listener.h"
#include "platform/inotifier.h"
#include "utils/misc.h"
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <utils/fmt_exception.h>

constexpr char MODULE_NAME[] = "InputListener";
constexpr char INPUT_DEV_DIR[] = "/dev/input";
constexpr int MIN_INTERVAL_US = MsToUs(2);
constexpr int POLL_RETRY_US = MsToUs(1000);
constexpr float SWIPE_INC_LIMIT = 0.2;
constexpr int DEV_DIR_WATCH_FLAG = Inotify::CREATE | Inotify::DELETE;

struct InputListenerDef {
    bool enable;
    double swipeThd;
    double gestureThdX;
    double gestureThdY;
    double gestureDelayTime;
    double holdEnterTime;
};

void ReadUnused(int fd) {
    char unusedBuf[1024];
    read(fd, unusedBuf, sizeof(unusedBuf));
}

InputListener::InputListener() : cancelPollFd_(-1), prevBtn_(0), swipeDist_(0.0), startX_(0.5), startY_(0.5) {
    swipeThd_ = 0.01;
    gestureThdX_ = 0.03;
    gestureThdY_ = 0.03;
    gestureDelayUs_ = 2.0;
    holdDelayUs_ = 1.0;

    InitReaders();

    using namespace std::placeholders;
    touchHandler_ = std::bind(&InputListener::HandleTouchInput, this, _1);
    btnHandler_ = std::bind(&InputListener::HandleBtnInput, this, _1);
}

InputListener::~InputListener() {}

void InputListener::Start(void) {
    using namespace std::placeholders;
    auto inoti = Inotifier::GetInstance();
    inoti->Add(INPUT_DEV_DIR, DEV_DIR_WATCH_FLAG, std::bind(&InputListener::OnInputDevUpdated, this, _1, _2));

    th_ = std::thread(std::bind(&InputListener::TouchEventThread, this));
    th_.detach();
}

void InputListener::TouchEventThread(void) {
    SetSelfThreadName(MODULE_NAME);
    for (;;) {
        FlushPendingReaders();
        int nr = poll(pollfds_.data(), pollfds_.size(), -1);
        if (nr <= 0) {
            Sleep(POLL_RETRY_US);
            continue;
        }
        for (const auto &pfd : pollfds_) {
            if ((pfd.revents & POLLIN) == 0) {
                continue;
            }
            HandleInput(pfd.fd);
        }
        Sleep(MIN_INTERVAL_US); // no sleep result MiPad 5 Pro deadloop
    }
}

void InputListener::OnInputDevUpdated(const std::string &filename, int flag) {
    auto path = fmt::format("{}/{}", INPUT_DEV_DIR, filename);
    switch (flag) {
        case Inotify::CREATE: {
            std::lock_guard _(mut_);
            addPending_.emplace_back(path);
            CancelPoll();
            break;
        }
        case Inotify::DELETE: {
            std::lock_guard _(mut_);
            removePending_.emplace_back(path);
            CancelPoll();
            break;
        }
        default:
            break;
    }
}

void InputListener::InitReaders(void) {
    PreparePoll();

    DIR *dirFp = opendir(INPUT_DEV_DIR);
    struct dirent *dp;
    while ((dp = readdir(dirFp))) {
        if (strchr(dp->d_name, '.')) {
            continue;
        }
        auto path = fmt::format("{}/{}", INPUT_DEV_DIR, dp->d_name);
        AddReader(path);
    }
    closedir(dirFp);
}

void InputListener::AddReader(const std::string &path) {
    for (const auto &[_, reader] : readers_) {
        const auto &prop = reader->GetProp();
        if (prop.path == path) {
            return;
        }
    }

    try {
        auto reader = std::make_unique<InputReader>(path);
        const auto &prop = reader->GetProp();
        readers_.emplace(prop.fd, std::move(reader));
        AddPoll(prop.fd);
        SPDLOG_DEBUG("Add {} '{}'", std::to_string(prop.devType), prop.name);
    } catch (const std::exception &e) {
        // SPDLOG_INFO("Unknown input device '{}'", path);
    }
}

void InputListener::RemoveReader(const std::string &path) {
    for (const auto &[fd, reader] : readers_) {
        const auto &prop = reader->GetProp();
        if (prop.path != path) {
            continue;
        }
        // clear prev input info from this device
        InputReader::Info info;
        switch (prop.devType) {
            case InputReader::DevType::TOUCH_PANEL:
                HandleTouchInput(info);
                break;
            case InputReader::DevType::BTN:
            case InputReader::DevType::MOUSE:
                HandleBtnInput(info);
                break;
            default:
                break;
        }
        SPDLOG_DEBUG("Remove {} '{}'", std::to_string(prop.devType), prop.name);
        RemovePoll(fd);
        readers_.erase(fd);
        return;
    }
}

void InputListener::FlushPendingReaders(void) {
    std::lock_guard _(mut_);
    for (const auto &p : removePending_) {
        RemoveReader(p);
    }
    for (const auto &p : addPending_) {
        AddReader(p);
    }
}

void InputListener::PreparePoll(void) {
    int fdPair[2];
    auto ret = pipe2(fdPair, O_CLOEXEC);
    if (ret) {
        throw FmtException("Failed to init InputListener event pipe");
    }

    AddPoll(fdPair[0]);
    cancelPollFd_ = fdPair[1];
}

void InputListener::AddPoll(int fd) {
    pollfd pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = fd;
    pfd.events = POLLIN;

    bool noUnused = true;
    for (auto &p : pollfds_) {
        if (p.fd < 0) {
            p = pfd;
            noUnused = false;
        }
    }
    if (noUnused) {
        pollfds_.emplace_back(pfd);
    }
}

void InputListener::RemovePoll(int fd) {
    for (auto &p : pollfds_) {
        if (p.fd == fd) {
            p.fd = -1;
            break;
        }
    }
}

void InputListener::CancelPoll(void) { write(cancelPollFd_, &cancelPollFd_, sizeof(cancelPollFd_)); }

void InputListener::HandleInput(int fd) {
    auto it = readers_.find(fd);
    if (it == readers_.end()) {
        ReadUnused(fd); // pipe fd
        return;
    }
    auto &reader = it->second;
    switch (reader->GetProp().devType) {
        case InputReader::DevType::TOUCH_PANEL:
            reader->Parse(touchHandler_);
            break;
        case InputReader::DevType::BTN:
            reader->Parse(btnHandler_);
            break;
        case InputReader::DevType::MOUSE:
            reader->Parse(btnHandler_); // EV_KEY only, EV_REL not used
            break;
        default:
            break;
    }
}

float Distance2D(float x1, float y1, float x2, float y2) {
    return std::sqrt(std::pow(x1 - x2, 2) + std::pow(y1 - y2, 2));
}

void InputListener::HandleTouchInput(const InputReader::Info &info) {
    auto distance = [](float x1, float y1, float x2, float y2) {
        return std::sqrt(std::pow(x1 - x2, 2) + std::pow(y1 - y2, 2));
    };

    if (info.pressed) {
        if (prevTouch_.pressed) {
            auto dist = distance(info.x, info.y, prevTouch_.x, prevTouch_.y);
            swipeDist_ += (dist < SWIPE_INC_LIMIT) ? dist : 0.0;
        } else {
            touchTimer_.Reset();
            swipeDist_ = 0.0;
            startX_ = info.x;
            startY_ = info.y;
        }
    }
    if (info.pressed != prevTouch_.pressed) {
        CoPublish("input.touch", &info.pressed);
    }
    prevTouch_ = info;

    classified_.inSwipe = swipeDist_ > swipeThd_;
    classified_.inHold = info.pressed && touchTimer_.ElapsedUs() > holdDelayUs_;
    classified_.inGesture = gestureTimer_.ElapsedUs() < gestureDelayUs_;
    if (classified_.inGesture == false && classified_.inSwipe && IsGestureStartPos(startX_, startY_)) {
        gestureTimer_.Reset();
        classified_.inGesture = true;
    }
    if (classified_ != prevClassified_) {
        SPDLOG_TRACE("input.state hold={} swipe={} gesture={}", classified_.inHold, classified_.inSwipe,
                     classified_.inGesture);
        CoPublish("input.state", &classified_);
    }
    prevClassified_ = classified_;
}

void InputListener::HandleBtnInput(const InputReader::Info &info) {
    auto pressedCnt = prevBtn_ + (info.pressed ? 1 : -1);
    pressedCnt = Cap(pressedCnt, 0, INT_MAX);
    if (pressedCnt * prevBtn_ == 0) {
        bool pressed = pressedCnt > 0;
        CoPublish("input.btn", &pressed);
    }
    prevBtn_ = pressedCnt;
}

bool InputListener::IsGestureStartPos(float x, float y) {
    auto inPadding = [](float x, float thd) { return x < thd || x > (1.0f - thd); };
    return inPadding(x, gestureThdX_) || inPadding(y, gestureThdY_);
}
