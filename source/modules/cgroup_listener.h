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
#include "utils/inotify.h"
#include "utils/time_counter.h"
#include <thread>

class CgroupListener {
public:
    CgroupListener();
    void Start(void);

private:
    void OnTaModified(const std::string &, int);
    void OnFgModified(const std::string &, int);
    void OnBgModified(const std::string &, int);
    void OnReModified(const std::string &, int);

    Inotify inoti_;
    std::thread th_;

    PidList ta_;
    PidList fg_;
    PidList bg_;
    PidList re_;

    TimeCounter taTimer_;
    TimeCounter fgTimer_;
    TimeCounter bgTimer_;
    TimeCounter reTimer_;
};
