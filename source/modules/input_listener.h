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

#pragma once

#include "cobridge_type.h"
#include "platform/module_base.h"
#include "utils/input_reader.h"
#include "utils/time_counter.h"
#include <map>
#include <memory>
#include <mutex>
#include <sys/poll.h>
#include <thread>

class InputListener : public ModuleBase {
public:
    InputListener();
    ~InputListener();
    void Start(void) override;

private:
    void TouchEventThread(void);
    void OnInputDevUpdated(const std::string &filename, int flag);

    void InitReaders(void);
    void AddReader(const std::string &path);
    void RemoveReader(const std::string &path);
    void FlushPendingReaders(void);

    void PreparePoll(void);
    void AddPoll(int fd);
    void RemovePoll(int fd);
    void CancelPoll(void);

    void HandleInput(int fd);
    void HandleTouchInput(const InputReader::Info &info);
    void HandleBtnInput(const InputReader::Info &info);
    bool IsGestureStartPos(float x, float y);

    float swipeThd_;
    float gestureThdX_;
    float gestureThdY_;
    int gestureDelayUs_;
    int holdDelayUs_;

    std::map<int, std::unique_ptr<InputReader>> readers_;
    std::vector<std::string> addPending_;
    std::vector<std::string> removePending_;
    std::vector<pollfd> pollfds_;
    int cancelPollFd_;
    std::thread th_;
    std::mutex mut_;

    InputReader::InfoHandler touchHandler_;
    InputReader::InfoHandler btnHandler_;

    InputReader::Info prevTouch_;
    int prevBtn_;
    InputData prevClassified_;
    InputData classified_;

    TimeCounter touchTimer_;
    TimeCounter gestureTimer_;
    float swipeDist_;
    float startX_;
    float startY_;
};
