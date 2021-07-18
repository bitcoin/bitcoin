package=native_llvm
$(package)_version=4397b7095d640f9b9426c4d0135e999c5a1de1c5
$(package)_download_path=https://github.com/llvm/llvm-project/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=3070adeab6556c32b7492b4bff17454aad9d7ea694347fbb73ecf49bbcaa017a

define $(package)_preprocess_cmds
  mkdir build
endef

define $(package)_config_cmds
  cd build && \
  $($(package)_cmake) -DCMAKE_BUILD_TYPE=Release \
    -DCLANG_DEFAULT_LINKER=lld \
    -DLLVM_ENABLE_PROJECTS='clang;libcxx;libcxxabi;lld;' \
    -DLLVM_INCLUDE_EXAMPLES=OFF \
    -DLLVM_INCLUDE_BENCHMARKS=OFF \
    -DLLVM_INCLUDE_TESTS=OFF \
    -DLLVM_TARGETS_TO_BUILD=X86 \
    -DLLVM_INSTALL_TOOLCHAIN_ONLY=ON \
    -DLLVM_INSTALL_BINUTILS_SYMLINKS=ON \
    -DLLVM_INSTALL_CCTOOLS_SYMLINKS=ON \
    -DCMAKE_SKIP_RPATH="ON" ../llvm
endef

define $(package)_build_cmds
  cd build && \
  $(MAKE)
endef

define $(package)_stage_cmds
  cd build && \
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm -rf lib/libc++abi.so* && \
  rm -rf share
endef
