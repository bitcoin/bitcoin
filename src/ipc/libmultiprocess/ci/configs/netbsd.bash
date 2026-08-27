CI_DESC="CI config for NetBSD"
CI_DIR=build-netbsd
export CXXFLAGS="-Werror -Wall -Wextra -Wpedantic -Wno-unused-parameter ${CXXFLAGS:-}"
CMAKE_ARGS=(-G Ninja)
BUILD_ARGS=(-k 0)
