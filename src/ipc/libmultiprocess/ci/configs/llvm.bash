CI_DESC="CI job using LLVM-based libraries and tools (clang, libc++, clang-tidy, iwyu) and testing Ninja"
CI_DIR=build-llvm
NIX_ARGS=(--arg enableLibcxx true)
export CXX=clang++
export CXXFLAGS="-Werror -Wall -Wextra -Wpedantic -Wthread-safety -Wno-unused-parameter"
CMAKE_ARGS=(
  -G Ninja
  -DMP_ENABLE_CLANG_TIDY=ON
  -DMP_ENABLE_IWYU=ON
)
BUILD_ARGS=(-k 0)
