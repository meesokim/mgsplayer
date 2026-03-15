#!/bin/bash
set -e

PROJECT_ROOT=$(pwd)
TOOLCHAIN=$PROJECT_ROOT/toolchain-mingw64.cmake
SDL2_PATH=$PROJECT_ROOT/SDL2-2.30.0/x86_64-w64-mingw32

echo "--- Building libkss for Windows ---"
cd libkss
mkdir -p build-win
cd build-win
cmake -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN \
      -DKSS_ENABLE_EXAMPLES=OFF \
      -DKSS_ENABLE_TESTS=OFF \
      ..
make -j$(nproc)
cd ../..

echo "--- Building mgsplayer for Windows ---"
mkdir -p build-win
cd build-win
cmake -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN \
      -DSDL2_PATH=$SDL2_PATH \
      ..
make -j$(nproc)
cd ..

echo "--- Preparing Windows Distribution ---"
mkdir -p dist-win
cp build-win/mgsplayer.exe dist-win/

echo "--- Done! ---"
echo "A fully static Windows binary is located in: dist-win/mgsplayer.exe"
