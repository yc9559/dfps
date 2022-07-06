cmake_minimum_required(VERSION 3.13)

add_library(spdlog INTERFACE)
target_include_directories(spdlog INTERFACE spdlog)
