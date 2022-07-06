cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SRCS
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/*.c"
)

set(INCS
    "${CMAKE_CURRENT_LIST_DIR}"
)

execute_process(COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_REPO_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
configure_file(version.c.in version.c @ONLY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/.git/index)

add_executable(dfps ${SRCS} ${CMAKE_CURRENT_BINARY_DIR}/version.c)
target_include_directories(dfps PRIVATE ${INCS})
target_link_libraries(dfps PRIVATE spdlog scn)

set(THIS_COMPILE_FLAGS
    -Wall -Werror -fdata-sections -ffunction-sections -fno-threadsafe-statics -fno-omit-frame-pointer
    -D_GNU_SOURCE -DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG
)

set(THIS_LINK_FLAGS
    -ffixed-x18 -Wl,--hash-style=both -fPIE -Wl,-exclude-libs,ALL -Wl,--gc-sections
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_link_libraries(dfps PRIVATE c++_static dl)
    list(APPEND THIS_COMPILE_FLAGS -O3 -fvisibility=hidden -fvisibility-inlines-hidden -flto)
    list(APPEND THIS_LINK_FLAGS -static -O3 -fvisibility=hidden -fvisibility-inlines-hidden -flto -Wl,--strip-all)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND THIS_COMPILE_FLAGS -fsanitize=address)
    list(APPEND THIS_LINK_FLAGS -fsanitize=address)
endif()

target_compile_options(dfps PRIVATE ${THIS_COMPILE_FLAGS} -D_GNU_SOURCE)
target_link_options(dfps PRIVATE ${THIS_LINK_FLAGS})
