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

#include "cobridge_type.h"
#include "utils/input_reader.h"
#include "utils/time_counter.h"
#include <map>
#include <sys/poll.h>
#include <thread>

class InputListener {
public:
    InputListener();
    void Start(void);

private:
    void InitReaders(void);
    void AddPoll(int fd);
    void HandleTouchInput(const InputReader::Info &info);
    void HandleBtnInput(const InputReader::Info &info);
    bool IsGestureStartPos(float x, float y);

    int inputSampleMs_;
    float swipeThd_;
    float gesturePctX_;
    float gesturePctY_;
    int gestureDelayMs_;
    int holdDelayMs_;

    std::map<int, InputReader> readers_;
    std::vector<pollfd> pollfds_;
    std::thread th_;

    bool touchPressed_;
    bool btnPressed_;
    InputData classified_;

    InputReader::Info prevTouch_;
    TimeCounter touchTimer_;
    TimeCounter gestureTimer_;
    float swipeDist_;
    float startX_;
    float startY_;
};
