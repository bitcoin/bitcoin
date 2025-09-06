CI_DESC="CI config for macOS"
CI_DIR=build-macos
export CXXFLAGS="-Werror -Wall -Wextra -Wpedantic -Wno-unused-parameter"
CMAKE_ARGS=(-G Ninja)
BUILD_ARGS=(-k 0)
