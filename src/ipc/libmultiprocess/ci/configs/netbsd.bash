CI_DESC="CI config for NetBSD"
CI_DIR=build-netbsd
export CXXFLAGS="-Werror -Wall -Wextra -Wpedantic -Wno-unused-parameter"
# Hardcode GCC 14, since default GCC versions installed by NetBSD are older
# and may not be compatible with libmultiprocess. GCC 14 was chosen because
# it's the latest compiler available on all versions of NetBSD that we test.
# Note that the GCC version specified here must match the version specified
# in pkg_add in ci.yml.
export CXX="/usr/pkg/gcc14/bin/g++"
CMAKE_ARGS=(-G Ninja)
BUILD_ARGS=(-k 0)
