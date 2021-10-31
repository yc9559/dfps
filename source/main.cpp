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

#include <getopt.h>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include "utils/backtrace.h"
#include "utils/inotify.h"
#include "utils/misc.h"
#include "utils/misc_android.h"

#include "modules/cgroup_listener.h"
#include "modules/dynamic_fps.h"
#include "modules/input_listener.h"
#include "modules/offscreen_monitor.h"
#include "modules/topapp_monitor.h"

static const std::string DAEMON_NAME = "Dfpsd";
static const std::string PROC_NAME = "Dfps";
static const std::string PROJECT_URL = "https://github.com/yc9559/dfps";
static const std::string VERSION = "21.10.31";
static constexpr int TERM_SIG = SIGUSR1;

std::string configFile;
std::string logFile;

static pid_t dfps_pid;
static pid_t dead_pid;
static pid_t new_pid;
static pid_t old_pid;

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

void DfpsMain(void) {
    try {
        InputListener input;
        input.Start();
        CgroupListener cgroup;
        cgroup.Start();
        TopappMonitor topapp;
        topapp.Start();
        OffscreenMonitor offscreen;
        offscreen.Start();
        DynamicFps dfps(configFile);
        dfps.Start();

        SPDLOG_INFO("Dfps is running");
        for (;;) {
            sleep(UINT32_MAX);
        }
    } catch (const std::exception &e) {
        SPDLOG_ERROR("Exception thrown '{}'", e.what());
        exit(EXIT_FAILURE);
    }
}

void DfpsSigHandler(int sig) {
    if (sig == TERM_SIG) {
        exit(EXIT_SUCCESS);
    }
    exit(EXIT_FAILURE);
}

void SetSigHandler(void) {
    SetDumpBacktraceAsCrashHandler();
    signal(TERM_SIG, DfpsSigHandler);
}

void StartNewDfps(void) {
    old_pid = dfps_pid;
    new_pid = fork();
    if (new_pid == 0) {
        SetSelfThreadName(PROC_NAME);
        SetSigHandler();
        DfpsMain();
    }

    sleep(1);
    if (new_pid != dead_pid) {
        dfps_pid = new_pid;
    } else {
        SPDLOG_INFO("Failed to start dfps(pid={})", new_pid);
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
                SPDLOG_ERROR("Dfps(pid={}) terminated unexpectedly, try to get tombstone", dead_pid);
                // wait tombstone generated
                sleep(1);
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

    SPDLOG_INFO("Dfps {}, by Matt Yang (yccy@outlook.com)", VERSION);

    Inotify inotify;
    inotify.Add(configFile, Inotify::CLOSE_WRITE, nullptr);

    StartNewDfps();

    for (;;) {
        inotify.WaitAndHandle();
        SPDLOG_INFO("Config file updated");
        StartNewDfps();
        if (new_pid != dead_pid) {
            if (old_pid) {
                SPDLOG_INFO("Succeeded to load new config file, terminate the old dfps");
                kill(old_pid, TERM_SIG);
            } else {
                SPDLOG_INFO("Succeeded to load new config file");
            }
        } else {
            SPDLOG_INFO("Failed to load new config file, the old dfps will keep running");
        }
    }
}

void PrintHelp(void) {
    std::cout << std::endl;
    std::cout << "Dfps " << VERSION << " by Matt Yang (yccy@outlook.com)" << std::endl;
    std::cout << "Dynamic screen refresh rate controller for Android 10+. Details see " << PROJECT_URL << std::endl;
    std::cout << "Usage: dfps [-o log_file] config_file" << std::endl;
    std::cout << std::endl;
}

void ParseOpt(int argc, char **argv) {
    static const char opt_string[] = "ho:";
    static const struct option long_opts[] = {
        {"help", no_argument, NULL, 'h'},
        {"outfile", required_argument, NULL, 'o'},
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
    ParseOpt(argc, argv);
    InitLogger();

    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        SetSelfThreadName(DAEMON_NAME);
        Daemon();
    }
    return EXIT_SUCCESS;
}
