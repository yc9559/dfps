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
#include "utils/time_counter.h"

class CgroupListener : public ModuleBase {
public:
    CgroupListener();
    ~CgroupListener();
    void Start(void) override;

private:
    class Updater {
    public:
        Updater(const std::string &path, const std::string &topic, int64_t delayMs, bool heavy);
        void Start(void);

    private:
        void ReadAndPub(void);

        PidList pl_;
        TimeCounter tm_;
        HeavyWorker::Handle hw_;
        DelayedWorker::Handle dw_;
        std::string path_;
        std::string topic_;
        int64_t delayUs_;
        bool heavy_;
    };

    void OnTaModified(const std::string &, int);
    void OnFgModified(const std::string &, int);
    void OnBgModified(const std::string &, int);
    void OnReModified(const std::string &, int);

    Updater ta_;
    Updater fg_;
    Updater bg_;
    Updater re_;
};
