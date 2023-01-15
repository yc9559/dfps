#!/bin/bash
#
# Copyright (C) 2021-2022 Matt Yang
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# stop on error
set -e

TOOLCHAIN_PREBUILT="$ANDROID_NDK/toolchains/llvm/prebuilt/linux-x86_64"
TOOLCHAIN_BIN="$TOOLCHAIN_PREBUILT/bin"
BASEDIR="$(dirname $(readlink -f "$0"))"
BUILD_DIR="$BASEDIR/build"
BUILD_TYPE="$1"
BUILD_TASKS="$2"
ARM64_PREFIX=aarch64-linux-android23
ARM32_PREFIX=armv7a-linux-androideabi23

# $1:prefix $2:targets
build_targets() {
    mkdir -p $BUILD_DIR/$1
    cmake \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DCMAKE_C_COMPILER="$TOOLCHAIN_BIN/$1-clang" \
        -DCMAKE_CXX_COMPILER="$TOOLCHAIN_BIN/$1-clang++" \
        -H$BASEDIR \
        -B$BUILD_DIR/$1 \
        -G "Unix Makefiles"
    cmake --build $BUILD_DIR/$1 --config $BUILD_TYPE --target $2 -j
}

# $1:prefix $2:targets $3:dst
copy_targets() {
    for t in $2; do
        cp -f $BUILD_DIR/$1/runnable/$t $3
    done
}

# $1:dst
copy_libs() {
    if [ "$BUILD_TYPE" == "Release" ]; then
        return
    elif [ "$BUILD_TYPE" == "Debug" ]; then
        cp -f $TOOLCHAIN_PREBUILT/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so $1
        cp -f $TOOLCHAIN_PREBUILT/lib64/clang/*/lib/linux/libclang_rt.asan-aarch64-android.so $1
    else
        cp -f $TOOLCHAIN_PREBUILT/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so $1
        if [ -f $TOOLCHAIN_BIN/aarch64-linux-android-strip ]; then
            $TOOLCHAIN_BIN/aarch64-linux-android-strip -s $1/libc++_shared.so
        else
            $TOOLCHAIN_BIN/llvm-strip -s $1/libc++_shared.so
        fi
    fi
}

make_dfps() {
    echo ">>> Making dfps binaries"
    build_targets $ARM64_PREFIX "dfps"
}

pack_dfps() {
    echo ">>> Packing dfps-magisk.zip"
    cd $BASEDIR/magisk

    copy_targets $ARM64_PREFIX dfps $BASEDIR/magisk/bin/dfps
    copy_libs $BASEDIR/magisk/bin
    cp -f $BASEDIR/LICENSE .
    cp -f $BASEDIR/NOTICE .

    zip dfps-magisk.zip -q -9 -r .
    mkdir -p $BUILD_DIR/package
    mv -f dfps-magisk.zip $BUILD_DIR/package

    rm -f bin/dfps
    rm -f bin/*.so
    rm -f LICENSE NOTICE
}

install_dfps() {
    echo ">>> Pushing dfps-magisk.zip to device"
    adb push $BUILD_DIR/package/dfps-magisk.zip /data/local/tmp
    echo ">>> Installing dfps on device"
    adb shell su -c magisk --install-module /data/local/tmp/dfps-magisk.zip
}

reboot_install_dfps() {
    echo ">>> Reboot device"
    adb shell reboot
}

clean() {
    rm -rf $BUILD_DIR
}

# $1:task
do_task() {
    case $1 in
    make)
        make_dfps
        ;;
    pack)
        pack_dfps
        ;;
    install)
        install_dfps
        ;;
    all)
        make_dfps
        pack_dfps
        install_dfps
        ;;
    reboot)
        reboot_install_dfps
        ;;
    clean)
        clean
        ;;
    *)
        echo " ! Unknown task name $1"
        exit 1
        ;;
    esac
}

for t in $BUILD_TASKS; do
    do_task $t
done
