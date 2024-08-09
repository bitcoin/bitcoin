# Bitcoin Tidy

Example Usage:

```bash
cmake -S . -B build -DLLVM_DIR=$(llvm-config --cmakedir) -DCMAKE_BUILD_TYPE=Release

cmake --build build -j$(getconf _NPROCESSORS_ONLN)

cmake --build build --target bitcoin-tidy-tests -j$(getconf _NPROCESSORS_ONLN)
```
