CI_DESC="CI job running ThreadSanitizer"
CI_DIR=build-sanitize
NIX_ARGS=(--arg enableLibcxx true --argstr libcxxSanitizers "Thread" --argstr capnprotoSanitizers "thread")
export CXX=clang++
export CXXFLAGS="-ggdb -Werror -Wall -Wextra -Wpedantic -Wthread-safety -Wno-unused-parameter -fsanitize=thread"
CMAKE_ARGS=()
BUILD_ARGS=(-k -j4)
BUILD_TARGETS=(mptest)
