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

#include "input_reader.h"
#include "fmt_exception.h"
#include "misc.h"
#include <cstring>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

enum KeyState { KEY_STATE_UP = 0, KEY_STATE_DOWN = 1 };

constexpr std::size_t INPUT_MAX_EVENTS = 1024;

InputReader::InputReader(const std::string &path) : buf_(sizeof(input_event) * INPUT_MAX_EVENTS) {
    prop_.fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (prop_.fd <= 0) {
        throw FmtException("Cannot open input device {}", path);
    }

    if (IsTouchPanel()) {
        ReadTouchpanelProp();
    } else if (IsBtnDev()) {
        ReadBtnProp();
    } else {
        throw FmtException("Unknown input device {}", path);
    }
}

InputReader::~InputReader() { close(prop_.fd); }

const InputReader::DevProp &InputReader::GetProp(void) const { return prop_; }

float LimitRange(int val, const std::pair<int, int> &range) {
    return static_cast<float>(val - range.first) / (range.second - range.first + 1);
}

void InputReader::Parse(const std::function<void(const Info &)> &onSync) {
    info_.ts = GetNowTs();

    size_t len = read(prop_.fd, buf_.data(), buf_.size());
    for (size_t i = 0; i < len; i += sizeof(input_event)) {
        const auto &evt = *reinterpret_cast<input_event *>(buf_.data() + i);
        switch (evt.type) {
            case EV_SYN:
                if (onSync) {
                    onSync(info_);
                }
                break;
            case EV_ABS:
                switch (evt.code) {
                    case ABS_MT_POSITION_X:
                        info_.x = LimitRange(evt.value, prop_.xRange);
                        break;
                    case ABS_MT_POSITION_Y:
                        info_.y = LimitRange(evt.value, prop_.yRange);
                        break;
                    case ABS_MT_TRACKING_ID:
                        if (prop_.useEvtKey == false) {
                            info_.pressed = (evt.value != UINT_MAX);
                        }
                        break;
                    default:
                        break;
                }
                break;
            case EV_KEY:
                switch (evt.code) {
                    case BTN_TOUCH:
                    case BTN_TOOL_FINGER:
                        info_.pressed = (evt.value == KEY_STATE_DOWN);
                        prop_.useEvtKey = true;
                        break;
                    default:
                        info_.pressed = (evt.value == KEY_STATE_DOWN);
                        break;
                }
                break;
            default:
                break;
        }
    }
}

bool InputReader::IsTouchPanel(void) {
    auto fd = prop_.fd;

    // support absolute position?
    unsigned long evbit = 0;
    ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), &evbit);
    bool hasAbsFeature = evbit & (1 << EV_ABS);
    if (hasAbsFeature == false) {
        return false;
    }

    // legal absolute position range?
    struct input_absinfo absinfo;
    memset(&absinfo, 0, sizeof(absinfo));
    ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absinfo);
    if (absinfo.minimum == absinfo.maximum) {
        return false;
    }
    memset(&absinfo, 0, sizeof(absinfo));
    ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absinfo);
    if (absinfo.minimum == absinfo.maximum) {
        return false;
    }

    return true;
}

bool InputReader::IsBtnDev(void) {
    auto fd = prop_.fd;

    // support key?
    unsigned long evbit = 0;
    ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), &evbit);
    bool hasKeyFeature = evbit & (1 << EV_KEY);
    if (hasKeyFeature == false) {
        return false;
    }

    return true;
}

void InputReader::ReadTouchpanelProp() {
    auto fd = prop_.fd;
    prop_.devType = DevType::TOUCH_PANEL;

    char name[64];
    ioctl(fd, EVIOCGNAME(sizeof(name)), name);
    prop_.name = name;

    struct input_absinfo absinfo;
    memset(&absinfo, 0, sizeof(absinfo));
    ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absinfo);
    prop_.xRange.first = absinfo.minimum;
    prop_.xRange.second = absinfo.maximum;
    memset(&absinfo, 0, sizeof(absinfo));
    ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absinfo);
    prop_.yRange.first = absinfo.minimum;
    prop_.yRange.second = absinfo.maximum;
}

void InputReader::ReadBtnProp() {
    auto fd = prop_.fd;
    prop_.devType = DevType::BTN;

    char name[64];
    ioctl(fd, EVIOCGNAME(sizeof(name)), name);
    prop_.name = name;
}

namespace std {

std::string to_string(InputReader::DevType type) {
    switch (type) {
        case InputReader::DevType::TOUCH_PANEL:
            return "touchpanel";
        case InputReader::DevType::BTN:
            return "btn";
        default:
            break;
    }
    return "unknown";
}

} // namespace std
