#!/usr/bin/env sh
exec /wasmcc/bin/clang --target=wasm32-wasi --sysroot /wasi-sysroot "$@"
