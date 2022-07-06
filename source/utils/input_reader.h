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

#include <functional>
#include <string>
#include <vector>

struct input_event;

class InputReader {
public:
    enum class DevType { TOUCH_PANEL = 0, BTN, MOUSE, NUM };

    struct Info {
        bool pressed;
        float x;
        float y;
        int64_t ts;
        Info() : pressed(false), x(0.0), y(0.0), ts(0) {}
    };

    using InfoHandler = std::function<void(const Info &)>;

    struct DevProp {
        int fd;
        DevType devType;
        bool useEvtKey;
        std::pair<int, int> xRange;
        std::pair<int, int> yRange;
        std::string name;
        std::string path;
        DevProp() : fd(0), devType(DevType::TOUCH_PANEL), useEvtKey(false) {}
    };

    explicit InputReader(const std::string &path);
    ~InputReader();

    const DevProp &GetProp(void) const;
    void Parse(const InfoHandler &handler);

private:
    bool IsTouchPanel(void);
    bool IsBtnDev(void);
    bool IsMouseDev(void);

    void ReadTouchpanelProp();
    void ReadBtnProp();
    void ReadMouseProp();

    void ParseTouchPanel(const input_event *evt, const InfoHandler &handler);
    void ParseBtn(const input_event *evt, const InfoHandler &handler);
    void ParseMouse(const input_event *evt, const InfoHandler &handler);

    DevProp prop_;
    Info info_;
    std::vector<uint8_t> buf_;
};

namespace std {

std::string to_string(InputReader::DevType type);

} // namespace std
