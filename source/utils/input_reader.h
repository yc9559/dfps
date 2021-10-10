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
#include <vector>

class InputReader {
public:
    enum class DevType { TOUCH_PANEL = 0, BTN };

    struct Info {
        bool pressed;
        float x;
        float y;
        int64_t ts;
        Info() : pressed(false), x(0.0), y(0.0), ts(0) {}
    };

    struct DevProp {
        int fd;
        std::string name;
        DevType devType;
        bool useEvtKey;
        std::pair<int, int> xRange;
        std::pair<int, int> yRange;
        DevProp() : fd(0), useEvtKey(false) {}
    };

    InputReader(const std::string &path);
    ~InputReader();

    const DevProp &GetProp(void) const;
    void Parse(const std::function<void(const Info &)> &onSync);

private:
    bool IsTouchPanel(void);
    bool IsBtnDev(void);
    void ReadTouchpanelProp();
    void ReadBtnProp();

    DevProp prop_;
    Info info_;
    std::vector<uint8_t> buf_;
};
