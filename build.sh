#!/bin/sh
set -eu
uname="$(uname | tr A-Z a-z)"
builddir="build-$uname-$(uname -m)"
[ -d "$builddir" ] || mkdir -m 0700 "$builddir"
find "$builddir" -depth -mindepth 1 | xargs rm -rf --
cd "$builddir"
echo "Entering directory '$PWD'"
export CC="${CC:-clang}"
export CFLAGS="${CFLAGS:--Werror -Wall -Wextra -g}"
set -x
cp -p ../usertcp_config_"$uname".h usertcp_config.h
$CC $CFLAGS -I . -o usertcp \
    ../usertcp_client.c \
    ../usertcp_nobody.c \
    ../usertcp_nobody_"$uname".c \
    ../usertcp_root.c \
    ../usertcp_root_"$uname".c \
    ../usertcp_util.c \
    ../sig.c
