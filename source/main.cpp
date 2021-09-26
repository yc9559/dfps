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
#include <string>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include "utils/inotify.h"

static const std::string DAEMON_NAME = "Dfpsd";
static const std::string PROC_NAME = "Dfps";
static const std::string PROJECT_URL = "https://github.com/yc9559/dfps";
static const std::string VERSION = "21.09.25";
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
    logger->set_pattern("[%H:%M:%S][%L] %!: %v");
    logger->flush_on(spdlog::level::info);
}

void DfpsMain(void) {
    SPDLOG_INFO("New dfps is running");
    for (;;) {
        sleep(UINT32_MAX);
    }
}

void DfpsSigHandler(int signnum) {
    if (signnum == TERM_SIG) {
        exit(EXIT_SUCCESS);
    }
    exit(EXIT_FAILURE);
}

void StartNewDfps(void) {
    old_pid = dfps_pid;
    new_pid = fork();
    if (new_pid == 0) {
        signal(TERM_SIG, DfpsSigHandler);
        signal(SIGTERM, NULL);
        signal(SIGINT, NULL);
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
                SPDLOG_ERROR("Dfps(pid={}) terminated unexpectedly", dead_pid);
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
            SPDLOG_INFO("Succeeded to load new config file, terminate the old dfps");
            kill(old_pid, TERM_SIG);
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
        prctl(PR_SET_NAME, DAEMON_NAME.c_str());
        Daemon();
    }
    return EXIT_SUCCESS;
}
