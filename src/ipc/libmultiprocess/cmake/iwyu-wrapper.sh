#!/usr/bin/env bash
# Run include-what-you-use for CMAKE_CXX_INCLUDE_WHAT_YOU_USE and make sure its
# output is shown when it fails. Usage: iwyu-wrapper.sh IWYU [ARGS...]
#
# "cmake -E __run_co_compile" only prints IWYU output that contains "should add
# these lines:" or "should remove these lines:" (HandleIWYU in CMake's
# Source/cmcmd.cxx). If IWYU fails for another reason, such as a clang error,
# the build stops with a bare "Error 1". In that case this script appends a line
# containing the marker text, so CMake prints the clang diagnostics too.
#
# This works around https://gitlab.kitware.com/cmake/cmake/-/issues/28020 and
# can be removed once CMake prints IWYU output whenever IWYU fails.
set -o nounset
iwyu=$1
shift
out=$("$iwyu" "$@" 2>&1)
ret=$?
printf '%s\n' "$out" >&2
if [ "$ret" -ne 0 ] && ! grep -q -e 'should add these lines:' -e 'should remove these lines:' <<<"$out"; then
  printf 'include-what-you-use failed with exit status %s before suggesting includes (marker for CMake: "should add these lines:"); see the diagnostics above.\n' "$ret" >&2
fi
exit "$ret"
