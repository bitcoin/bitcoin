CI_DESC="CI job using old Cap'n Proto version"
CI_DIR=build-olddeps
export CXXFLAGS="-Werror -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-error=array-bounds"
NIX_ARGS=(--argstr capnprotoVersion "0.7.1")
BUILD_ARGS=(-k)
