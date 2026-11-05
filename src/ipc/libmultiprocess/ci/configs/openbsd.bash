CI_DESC="CI config for OpenBSD"
CI_DIR=build-openbsd
# The -Wno-c2y-extensions flag can be dropped once the change
# from commit 3724713e82456ce7d66d1f377539dd2e142d969a becomes
# available in Clang.
export CXXFLAGS="-Werror -Wall -Wextra -Wpedantic -Wunused -Wunused-const-variable -Wno-c++23-lambda-attributes -Wno-c2y-extensions"
CMAKE_ARGS=(-G Ninja)
BUILD_ARGS=(-k 0)
