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

#!/vendor/bin/sh

BASEDIR="$(dirname $(readlink -f "$0"))"

wait_until_login() {
    # in case of /data encryption is disabled
    while [ "$(getprop sys.boot_completed)" != "1" ]; do
        sleep 1
    done

    # we doesn't have the permission to rw "/sdcard" before the user unlocks the screen
    local test_file="/sdcard/Android/.PERMISSION_TEST"
    true >"$test_file"
    while [ ! -f "$test_file" ]; do
        true >"$test_file"
        sleep 1
    done
    rm "$test_file"
}

wait_until_login

DFPS_DIR="/sdcard/Android/yc/dfps"
if [ -f $BASEDIR/bin/libc++_shared.so ]; then
    ASAN_LIB="$(ls $BASEDIR/bin/libclang_rt.asan-*-android.so)"
    export LD_PRELOAD="$ASAN_LIB $BASEDIR/bin/libc++_shared.so"
fi
$BASEDIR/bin/dfps $DFPS_DIR/dfps.txt -o $DFPS_DIR/dfps_log.txt -n $DFPS_DIR/dfps_cur.txt
