cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
add_definitions(-D_GNU_SOURCE)
add_compile_options(-Wall -Werror -fdata-sections -ffunction-sections -fno-threadsafe-statics)
set(LINKER_FLAGS "-static -ffixed-x18 -Wl,--hash-style=both -fPIE -Wl,-exclude-libs,ALL -Wl,--gc-sections")

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3 -flto -fvisibility=hidden -fvisibility-inlines-hidden)
    set(LINKER_FLAGS "${LINKER_FLAGS} -O3 -flto -fvisibility=hidden -fvisibility-inlines-hidden -Wl,--strip-all")
else ()
    add_definitions(-DDEBUG)
endif ()

file(GLOB_RECURSE SRCS
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/*.c"
)

set(INCS
    "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/thirdparty"
)

add_executable(dfps ${SRCS})
set_target_properties(dfps PROPERTIES LINK_FLAGS ${LINKER_FLAGS})
target_include_directories(dfps PRIVATE ${INCS})
target_link_libraries(dfps PRIVATE c++_static m c dl)

add_custom_target(dfps_packing
    COMMENT "Packing dfps-magisk.zip"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/magisk
    COMMAND mkdir -p ${CMAKE_SOURCE_DIR}/magisk/bin
    COMMAND cp -f $<TARGET_FILE:dfps> ${CMAKE_SOURCE_DIR}/magisk/bin
    COMMAND zip dfps-magisk.zip -q -9 -r .
    COMMAND mkdir -p ${CMAKE_BINARY_DIR}/package
    COMMAND mv -f ${CMAKE_SOURCE_DIR}/magisk/dfps-magisk.zip ${CMAKE_BINARY_DIR}/package
    COMMAND rm -f ${CMAKE_SOURCE_DIR}/magisk/bin/$<TARGET_FILE_NAME:dfps>
    DEPENDS dfps
)

add_custom_target(dfps_pushing
    COMMENT "Pushing dfps-magisk.zip to device"
    COMMAND adb push ${CMAKE_BINARY_DIR}/package/dfps-magisk.zip /sdcard/Download
    DEPENDS dfps_packing
)
