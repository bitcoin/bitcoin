CI_DESC="CI job using old Cap'n Proto and cmake versions"
CI_DIR=build-olddeps
# Pin olddeps to an older Nixpkgs channel, since compiling the old CMake
# requires an older GCC.
NIXPKGS_CHANNEL=nixos-25.05
export CXXFLAGS="-Werror -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-error=array-bounds"
NIX_ARGS=(--argstr capnprotoVersion "0.9.2" --argstr cmakeVersion "3.12.4" --argstr gccVersion "11")
BUILD_ARGS=(-k)
