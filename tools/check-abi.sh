#!/bin/sh

set -eu

default_base_version="$(git describe --match "v*.*.*" --abbrev=0)"
default_new_version="HEAD"

display_help_and_exit() {
    echo "Usage: $0 [<base_ver> [<new_ver>]]"
    echo ""
    echo "Description: This script uses the ABI Compliance Checker tool to determine if the ABI"
    echo "             of a new version of libsecp256k1 has changed in a backward-incompatible way."
    echo ""
    echo "Options:"
    echo "  base_ver      Specify the base version as a git commit-ish"
    echo "                  (default: most recent reachable tag matching \"v.*.*\", currently \"$default_base_version\")"
    echo "  new_ver       Specify the new version as a git commit-ish"
    echo "                  (default: $default_new_version)"
    echo "  -h, --help    Display this help message"
    exit 0
}

if [ "$#" -eq 0 ]; then
    base_version="$default_base_version"
    new_version="$default_new_version"
elif [ "$#" -eq 1 ] && { [ "$1" = "-h" ] || [ "$1" = "--help" ]; }; then
    display_help_and_exit
elif [ "$#" -eq 1 ] || [ "$#" -eq 2 ]; then
    base_version="$1"
    if [ "$#" -eq 2 ]; then
        new_version="$2"
    fi
else
    echo "Invalid usage. See help:"
    echo ""
    display_help_and_exit
fi

checkout_and_build() {
    _orig_dir="$(pwd)"
    git worktree add --detach "$1" "$2"
    cd "$1"
    mkdir build && cd build
    cmake -S .. --preset dev-mode \
        -DCMAKE_C_COMPILER=gcc -DCMAKE_BUILD_TYPE=None -DCMAKE_C_FLAGS="-g -Og -gdwarf-4" \
        -DSECP256K1_BUILD_BENCHMARK=OFF \
        -DSECP256K1_BUILD_TESTS=OFF \
        -DSECP256K1_BUILD_EXHAUSTIVE_TESTS=OFF \
        -DSECP256K1_BUILD_CTIME_TESTS=OFF \
        -DSECP256K1_BUILD_EXAMPLES=OFF
    cmake --build . -j "$(nproc)"
    abi-dumper src/libsecp256k1.so -o ABI.dump -lver "$2" -public-headers ../include/
    cd "$_orig_dir"
}

echo "Comparing $base_version (base version) to $new_version (new version)"
echo

base_source_dir="$(mktemp -d)"
checkout_and_build "$base_source_dir" "$base_version"

new_source_dir="$(mktemp -d)"
checkout_and_build "$new_source_dir" "$new_version"

abi-compliance-checker -lib libsecp256k1 -old "${base_source_dir}/build/ABI.dump" -new "${new_source_dir}/build/ABI.dump"
git worktree remove "$base_source_dir"
git worktree remove "$new_source_dir"
