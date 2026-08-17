CI_DESC="CI job using default libraries and tools, and running IWYU"
CI_DIR=build-default
export CXXFLAGS="-Werror -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wextra-semi -Wmissing-noreturn -Wtrailing-whitespace"
CMAKE_ARGS=(-DMP_ENABLE_IWYU=ON)
BUILD_ARGS=(-k)
