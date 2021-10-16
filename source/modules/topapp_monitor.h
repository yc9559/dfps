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
#include "utils/delayed_worker.h"
#include "utils/heavy_worker.h"
#include "utils/time_counter.h"

class TopappMonitor {
public:
    TopappMonitor();
    void Start(void);

private:
    void OnCgroupUpdated(void *data);
    void OnTopappList(void *data);

    int curTopTaskNr_;
    int prevTopTaskNr_;
    TimeCounter timer_;
    HeavyWorker::Handle hw_;
    DelayedWorker::Handle dw_;
};
