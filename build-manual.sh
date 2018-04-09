#!/bin/sh -
set -eu
cd "$(dirname "$0")"
asciidoctor -b manpage "$1"
[ -n "${1:-}" ] && nroff -man "${1%.adoc}" | ${MANPAGER:-less -R}
