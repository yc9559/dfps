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

#include <getopt.h>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include "dfps.h"
#include "utils/inotify.h"
#include "utils/misc.h"
#include "utils/misc_android.h"
#include "version.h"

constexpr char PROC_NAME[] = "dfps";
constexpr char AUTHOR[] = "Matt Yang (yccy@outlook.com)";
constexpr char HELP_DESC[] =
    "Dynamic screen refresh rate controller for Android 10+. Details see https://github.com/yc9559/dfps.\n"
    "Usage: dfps [-o log_file] config_file";
constexpr char VERSION[] = "v1(21.10.31)";
constexpr int TERM_SIG = SIGUSR1;

std::string configFile;
std::string logFile;
std::string notifyFile;

static pid_t dead_pid;
static pid_t new_pid;
static pid_t old_pid;

Dfps dfps;

void InitLogger(void) {
    auto logger = spdlog::default_logger();
    if (logFile.empty() == false) {
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile, false);
        logger->sinks().emplace_back(sink);
    }
    // use SPDLOG_ACTIVE_LEVEL to switch log level
    logger->set_pattern("%H:%M:%S %L %v");
    logger->set_level(spdlog::level::debug);
    logger->flush_on(spdlog::level::debug);
}

void PrintTombstone(int pid) {
    auto bt = GetTombstone(pid);
    if (bt.empty()) {
        SPDLOG_ERROR("Cannot find the tombstone");
        return;
    }

    SPDLOG_ERROR(">>> Start of tombstone {} <<<", pid);
    std::stringstream ss(bt);
    std::string line;
    while (std::getline(ss, line)) {
        SPDLOG_ERROR(line);
    }
    SPDLOG_ERROR(">>> End of tombstone {} <<<", pid);
}

void AppMainMayThrow(void) {
    dfps.Load(configFile, notifyFile);
    dfps.Start();
    for (;;) {
        Sleep(UINT32_MAX);
    }
}

void AppMain(void) {
    try {
        AppMainMayThrow();
    } catch (const std::exception &e) {
        SPDLOG_ERROR("Exception thrown: {}", e.what());
        exit(EXIT_FAILURE);
    }
}

void AppSigHandler(int sig) {
    if (sig == TERM_SIG) {
        exit(EXIT_SUCCESS);
    }
    exit(EXIT_FAILURE);
}

void SetSigHandler(void) { signal(TERM_SIG, AppSigHandler); }

void StartNewApp(void) {
    new_pid = fork();
    if (new_pid == 0) {
        SetSigHandler();
        AppMain();
    }

    Sleep(SToUs(1.0));
    if (new_pid != dead_pid) {
        old_pid = new_pid;
    } else {
        SPDLOG_INFO("Failed to start {}(pid={})", PROC_NAME, new_pid);
    }
}

void KillOldApp(void) {
    if (old_pid) {
        kill(old_pid, TERM_SIG);
    }
}

void DaemonSigHandler(int signum) {
    pid_t child_pid;
    int status;
    switch (signum) {
        case SIGCHLD:
            child_pid = wait(&status);
            if (child_pid != new_pid && child_pid != old_pid) {
                return;
            }
            dead_pid = child_pid;
            if (WIFSIGNALED(status)) {
                SPDLOG_ERROR("{}(pid={}) terminated unexpectedly, try to get tombstone", PROC_NAME, dead_pid);
                // wait tombstone generated
                Sleep(SToUs(1.0));
                PrintTombstone(dead_pid);
            }
            break;
        case SIGTERM:
        case SIGINT:
            SPDLOG_INFO("Terminated by user");
            exit(EXIT_SUCCESS);
        default:
            break;
    }
}

void Daemon(void) {
    signal(SIGCHLD, DaemonSigHandler);
    signal(SIGTERM, DaemonSigHandler);
    signal(SIGINT, DaemonSigHandler);

    SPDLOG_INFO("{} {}[{}], by {}", PROC_NAME, VERSION, GetGitCommitHash(), AUTHOR);

    Inotify inotify;
    inotify.Add(configFile, Inotify::CLOSE_WRITE, nullptr);

    StartNewApp();

    for (;;) {
        inotify.WaitAndHandle();
        SPDLOG_INFO("Config file updated, restart {} to load new config file", PROC_NAME);
        KillOldApp();
        Sleep(SToUs(0.5)); // wait finishing termination
        StartNewApp();
    }
}

void PrintHelp(void) { std::cout << std::endl << HELP_DESC << std::endl; }

void ParseOpt(int argc, char **argv) {
    static const char opt_string[] = "ho:n:";
    static const struct option long_opts[] = {
        {"help", no_argument, NULL, 'h'},
        {"outfile", required_argument, NULL, 'o'},
        {"notify", required_argument, NULL, 'n'},
        {NULL, 0, 0, 0},
    };
    int opt;
    while ((opt = getopt_long(argc, argv, opt_string, long_opts, NULL)) != -1) {
        switch (opt) {
            case 'h':
                PrintHelp();
                exit(EXIT_SUCCESS);
                break;
            case 'o':
                logFile = std::string(optarg);
                remove(logFile.c_str());
                break;
            case 'n':
                notifyFile = std::string(optarg);
                break;
            default:
                PrintHelp();
                exit(EXIT_FAILURE);
                break;
        }
    }
    int len = argc - optind;
    if (len < 1) {
        SPDLOG_ERROR("Config file not specified");
        exit(EXIT_FAILURE);
    } else {
        configFile = argv[optind];
        if (access(configFile.c_str(), R_OK) != 0) {
            SPDLOG_ERROR("Config file not found");
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char **argv) {
    InitLogger();
    InitArgv(argc, argv);
    ParseOpt(argc, argv);
    InitLogger();

    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        SetSelfName(PROC_NAME);
        Daemon();
    }
    return EXIT_SUCCESS;
}
