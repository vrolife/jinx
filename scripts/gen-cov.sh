#!/bin/sh
set -e

CC="$1"
CXX="$2"
GCOV="$(which gcov)"

if "$CXX" -v 2>&1|grep -i clang 1>/dev/null 2>&1; then
    if test ! -e "$PWD/gcov"; then
        if which llvm-cov >/dev/null 2>&1; then
            ln -s $(which llvm-cov) ./gcov
        else
            ln -s $(which llvm-cov-$(echo __clang_major__ | clang++ -E -|grep -v '#')) ./gcov
        fi
    fi
    GCOV="$PWD/gcov"
fi

find . -name "*.gcda" -exec rm -f {} \;

cmake --build .

ctest

lcov \
    --gcov-tool "$GCOV" \
    --exclude "*/thirdparty/*" \
    --exclude "/usr/include/*" \
    --exclude "/usr/local/*" \
    --exclude "/Applications/*" \
    -c -d . -o cov.info-file 

mkdir -p html

genhtml -o html cov.info-file 
