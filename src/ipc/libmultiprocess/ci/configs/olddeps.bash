CI_DESC="CI job using old Cap'n Proto and cmake versions"
CI_DIR=build-olddeps
# Pin olddeps to an older Nixpkgs channel, to be able to test an older GCC.
CI_NIXPKGS_CHANNEL=nixos-25.05
export CXXFLAGS="-Werror -Wall -Wextra -Wpedantic -Wunused-const-variable -Wno-unused-parameter -Wno-error=array-bounds"
CAPNP_CHECKOUT=v0.9.2  # Use a checkout to compile it with the selected GCC
NIX_ARGS=(--argstr capnprotoVersion "none" --argstr cmakeVersion "3.12.4" --argstr gccVersion "10")
BUILD_ARGS=(-k)
