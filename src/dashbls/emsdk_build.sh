#!/usr/bin/env bash

git submodule update --init --recursive

rm -rf js_build
mkdir -p js_build
cd js_build

emcmake cmake -G "Unix Makefiles" ..
emmake make
