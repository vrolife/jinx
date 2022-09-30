#!/bin/sh

JINX_DIR="$PWD"

mkdir -p out/static
cd out/static

TOP_DIR="$PWD"

git clone --depth=1 https://github.com/libevent/libevent.git -b release-2.1.12-stable

cd libevent
cmake -G Ninja \
    -S "$TOP_DIR/libevent" \
    -B "$TOP_DIR/libevent-build" \
    -DCMAKE_INSTALL_PREFIX="$TOP_DIR/libevent-install" \
    -DCMAKE_C_COMPILER=musl-gcc \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_VISIBILITY_PRESET=hidden \
    -DEVENT__DISABLE_OPENSSL=ON \
    -DEVENT__DISABLE_DEBUG_MODE=ON \
    -DEVENT__DISABLE_THREAD_SUPPORT=ON \
    -DEVENT__DISABLE_BENCHMARK=ON \
    -DEVENT__DISABLE_TESTS=ON \
    -DEVENT__DISABLE_REGRESS=ON \
    -DEVENT__DISABLE_SAMPLES=ON

cmake --build "$TOP_DIR/libevent-build" --target install

cd "$TOP_DIR"

export REALGCC=g++

cmake -G Ninja \
    -S "$JINX_DIR" \
    -B "$TOP_DIR/jinx-build" \
    -DCMAKE_INSTALL_PREFIX="$TOP_DIR/jinx-install" \
    -DCMAKE_C_COMPILER=cc \
    -DCMAKE_CXX_COMPILER=musl-gcc \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_VISIBILITY_PRESET=hidden \
    -DJINX_NO_TESTS=ON \
    -DLIBEVENT_STATIC_LINK=ON \
    -DCMAKE_PREFIX_PATH="$TOP_DIR/libevent-install" \
    -DLIBEVENT_FIND_PACKAGE=ON

cmake --build "$TOP_DIR/jinx-build"
