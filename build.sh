#!/bin/sh
set -eu
uname="$(uname | tr A-Z a-z)"
builddir="build-$uname-$(uname -m)"
[ -d "$builddir" ] || mkdir -m 0700 "$builddir"
find "$builddir" -depth -mindepth 1 | xargs rm -rf --
cd "$builddir"
echo "Entering directory '$PWD'"
export CC="${CC:-clang}"
export CFLAGS="${CFLAGS:--Werror -Wall -Wextra -pedantic -std=c99 -g}"
set -x
cp -p ../config_"$uname".h config.h
$CC $CFLAGS -I . -o privatetcp \
    ../client.c \
    ../nobody.c \
    ../nobody_"$uname".c \
    ../root.c \
    ../root_"$uname".c \
    ../sig.c \
    ../util.c
