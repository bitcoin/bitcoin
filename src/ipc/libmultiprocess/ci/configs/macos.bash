CI_DESC="CI config for macOS"
CI_DIR=build-macos
export CXXFLAGS="-Werror -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-c++23-lambda-attributes"
CMAKE_ARGS=(-G Ninja -DMP_ENABLE_CLANG_TIDY=ON)
BUILD_ARGS=(-k 0)
