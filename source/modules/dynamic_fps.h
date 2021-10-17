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

#include <map>
#include <string>
#include "utils/delayed_worker.h"
#include "utils/heavy_worker.h"

class DynamicFps {
public:
    explicit DynamicFps(const std::string &configPath);
    void Start(void);

private:
    struct FpsRule {
        int idle;
        int active;
    };

    void LoadConfig(const std::string &configPath);
    void ParseLine(const std::string &line);
    void AddRule(const std::string &pkgName, FpsRule rule);
    void SetTunable(const std::string &tunable, const std::string &value);
    std::string FindInvalidRule(void);

    void AddReactor(void);
    void OnInputTouch(const void *data);
    void OnInputBtn(const void *data);
    void OnInput(void);
    void OnInputScene(const void *data);
    void OnTopAppSwitch(const void *data);
    void OnOffscreen(const void *data);

    void SwitchRefreshRate(void);
    void SwitchRefreshRate(int hz);

    bool useSfBackdoor_;
    int64_t touchSlackMs_;
    int64_t gestureSlackMs_;

    std::map<std::string, FpsRule> rules_;
    FpsRule offscreen_;
    FpsRule universial_;
    bool hasUniversial_;
    bool hasOffscreen_;

    bool touchPressed_;
    bool btnPressed_;
    bool wakeupBoost_;
    bool active_;
    std::string curApp_;
    std::string overridedApp_;
    bool isOffscreen_;
    int curHz_;
    bool forceSwitch_;

    DelayedWorker::Handle dwInput_;
    DelayedWorker::Handle dwGesture_;
    HeavyWorker::Handle hw_;
};
