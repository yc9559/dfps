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

#include "platform/module_base.h"

class Dfps {
public:
    Dfps();
    ~Dfps();

    void Load(const std::string &configPath, const std::string &notifyPath);
    void Start(void);

private:
    void SetSelfSchedHint(void);

    std::string configPath_;
    std::string notifyPath_;
    std::vector<std::unique_ptr<ModuleBase>> modules_;
};
