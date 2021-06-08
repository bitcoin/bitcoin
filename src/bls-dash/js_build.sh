#!/usr/bin/env bash

git submodule update --init --recursive

mkdir js_build
cd js_build

cmake ../ -DCMAKE_TOOLCHAIN_FILE=$(realpath $(which emcc))/cmake/Modules/Platform/Emscripten.cmake
cmake --build . --
