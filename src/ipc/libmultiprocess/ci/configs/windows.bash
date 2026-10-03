CI_DESC="CI job cross-compiling to Windows with MinGW, tested with Wine"
CI_DIR=build-windows
CI_CACHE_NIX_STORE=true
NIX_ARGS=(
  --arg windows true
  --arg minimal true
)
CMAKE_ARGS=(
  -G Ninja
  -DCMAKE_SYSTEM_NAME=Windows
  -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER
  -DCMAKE_CROSSCOMPILING_EMULATOR=wine
  # -Wa,-mbig-obj: template-heavy C++ generates >32K COFF sections per .obj;
  # BigCOFF format raises the limit.  Must be passed to the assembler via -Wa.
  "-DCMAKE_CXX_FLAGS=-Wa,-mbig-obj"
)
# CXX is set by the nix cross shell to the mingw g++ wrapper; cmake picks
# it up from the environment, so no CMAKE_CXX_COMPILER override needed.
BUILD_ARGS=(-k 0)
# Build native mpgen first (as a code generator for cross-compiled test/example targets).
MPGEN_PRE_BUILD=1
