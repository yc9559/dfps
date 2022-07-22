# Dfps
# Copyright (C) 2021 Matt Yang(yccy@outlook.com)

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
