#!/system/bin/sh

BASEDIR="$(dirname $(readlink -f "$0"))"

sh $BASEDIR/initsvc.sh
