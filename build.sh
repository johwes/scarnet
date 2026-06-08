#!/bin/bash
# OSS-Fuzz compatible build script.
# Environment variables follow the OSS-Fuzz convention:
#   $SRC  — root of this repository
#   $OUT  — directory for build outputs
#   $CC   — C compiler
#   $CFLAGS — compiler flags (set by the fuzzing infrastructure)
#   $LIB_FUZZING_ENGINE — linker flag for the fuzzing engine

set -euo pipefail

SRC="${SRC:-.}"
OUT="${OUT:-./build}"
CC="${CC:-clang}"
CFLAGS="${CFLAGS:--O0 -g}"
LIB_FUZZING_ENGINE="${LIB_FUZZING_ENGINE:--lFuzzer}"

mkdir -p "$OUT"

SOURCES="$SRC/src/parse.c $SRC/src/handler.c $SRC/src/session.c $SRC/src/util.c"

echo "Compiling scarnet sources..."
OBJS=""
for f in $SOURCES; do
    obj="$OUT/$(basename "${f%.c}").o"
    $CC $CFLAGS -I"$SRC/include" -c "$f" -o "$obj"
    OBJS="$OBJS $obj"
done

echo "Linking fuzz target..."
$CC $CFLAGS -I"$SRC/include" \
    $OBJS "$SRC/fuzz_target.c" \
    $LIB_FUZZING_ENGINE \
    -o "$OUT/scarnet_fuzz"

echo "Done: $OUT/scarnet_fuzz"
