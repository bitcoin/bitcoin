CI_DESC="CI job cross-compiling to 32-bit"
CI_DIR=build-gnu32
NIX_ARGS=(
  --arg minimal true
  --arg crossPkgs 'import <nixpkgs> { crossSystem = { config = "i686-unknown-linux-gnu"; }; }'
)
export CXXFLAGS="-Werror -Wall -Wextra -Wpedantic -Wno-unused-parameter"
CMAKE_ARGS=(-G Ninja)
BUILD_ARGS=(-k 0)
