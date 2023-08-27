#!/usr/bin/env sh
set -e

CFLAGS="-Wall -Wextra -pedantic -std=c11"
CLIBS="-lraylib -lm"

gcc $CFLAGS -o quavatar main.c $CLIBS

if [ "$1" = "run" ]
then
    shift
    ./quavatar "$@"
fi
