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

#include "input_listener.h"
#include "utils/cobridge.h"
#include "utils/misc.h"
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/stat.h>

constexpr char MODULE_NAME[] = "InputListener";
constexpr char INPUT_DEV_DIR[] = "/dev/input";
constexpr int INPUT_LISTEN_RETRY_MS = 1000;
constexpr int INPUT_SAMPLE_MS = 10;
constexpr float SWIPE_THD = 0.03;
constexpr float GESTURE_PCT_X = 0.04;
constexpr float GESTURE_PCT_Y = 0.03;
constexpr int GESTURE_DELAY_MS = 1000;
constexpr int HOLD_DELAY_MS = 2000;

InputListener::InputListener()
    : inputSampleMs_(INPUT_SAMPLE_MS),
      swipeThd_(SWIPE_THD),
      gesturePctX_(GESTURE_PCT_X),
      gesturePctY_(GESTURE_PCT_Y),
      gestureDelayMs_(GESTURE_DELAY_MS),
      holdDelayMs_(HOLD_DELAY_MS),
      touchPressed_(false),
      btnPressed_(false),
      swipeDist_(0.0),
      startX_(0.5),
      startY_(0.5) {
    InitReaders();
}

void InputListener::Start(void) {
    using namespace std::placeholders;
    auto touchHandler = std::bind(&InputListener::HandleTouchInput, this, _1);
    auto btnHandler = std::bind(&InputListener::HandleBtnInput, this, _1);

    th_ = std::thread([=]() {
        SetSelfThreadName(MODULE_NAME);
        for (;;) {
            int nr = poll(pollfds_.data(), pollfds_.size(), -1);
            if (nr <= 0) {
                usleep(MsToUs(INPUT_LISTEN_RETRY_MS));
                continue;
            }
            for (const auto &pfd : pollfds_) {
                if ((pfd.revents & POLLIN) == 0) {
                    continue;
                }
                int fd = pfd.fd;
                auto &reader = readers_.at(fd);
                switch (reader.GetProp().devType) {
                    case InputReader::DevType::TOUCH_PANEL:
                        reader.Parse(touchHandler);
                        break;
                    case InputReader::DevType::BTN:
                        reader.Parse(btnHandler);
                        break;
                    default:
                        break;
                }
            }
            usleep(MsToUs(inputSampleMs_));
        }
    });
    th_.detach();
}

void InputListener::InitReaders(void) {
    DIR *dirFp = opendir(INPUT_DEV_DIR);
    struct dirent *dp;
    char path[PATH_MAX];
    while ((dp = readdir(dirFp))) {
        if (strchr(dp->d_name, '.')) {
            continue;
        }
        snprintf(path, sizeof(path), "%s/%s", INPUT_DEV_DIR, dp->d_name);
        try {
            InputReader reader(path);
            int fd = reader.GetProp().fd;
            readers_.emplace(fd, reader);
            AddPoll(fd);
            SPDLOG_INFO("Listening {} device '{}'", std::to_string(reader.GetProp().devType), path);
        } catch (const std::exception &e) {
            // SPDLOG_INFO("Skip unsupported input device '{}'", path);
        }
    }
    closedir(dirFp);
}

void InputListener::AddPoll(int fd) {
    pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;
    pollfds_.emplace_back(pfd);
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
            swipeDist_ += distance(info.x, info.y, prevTouch_.x, prevTouch_.y);
        } else {
            touchTimer_.Reset();
            swipeDist_ = 0.0;
            startX_ = info.x;
            startY_ = info.y;
        }
    }
    prevTouch_ = info;
    touchPressed_ = info.pressed;
    CoBridge::GetInstance()->Publish("input.touch", &touchPressed_);

    classified_.inSwipe = swipeDist_ > swipeThd_;
    classified_.inHold = touchPressed_ && touchTimer_.ElapsedMs() > holdDelayMs_;
    classified_.inGesture = gestureTimer_.ElapsedMs() < gestureDelayMs_;
    if (classified_.inGesture == false && classified_.inSwipe && IsGestureStartPos(startX_, startY_)) {
        gestureTimer_.Reset();
        classified_.inGesture = true;
    }
    CoBridge::GetInstance()->Publish("input.state", &classified_);
}

void InputListener::HandleBtnInput(const InputReader::Info &info) {
    btnPressed_ = info.pressed;
    CoBridge::GetInstance()->Publish("input.btn", &btnPressed_);
}

bool InputListener::IsGestureStartPos(float x, float y) {
    auto inPadding = [](float x, float thd) { return x < thd || x > (1.0f - thd); };
    return inPadding(x, gesturePctX_) || inPadding(y, gesturePctY_);
}
