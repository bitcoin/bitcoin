package=native_llvm
$(package)_version=17.0.6
$(package)_major_version=$(firstword $(subst ., ,$($(package)_version)))
$(package)_download_path=https://github.com/llvm/llvm-project/releases/download/llvmorg-$($(package)_version)
ifneq (,$(findstring aarch64,$(BUILD)))
$(package)_file_name=clang+llvm-$($(package)_version)-aarch64-linux-gnu.tar.xz
$(package)_sha256_hash=6dd62762285326f223f40b8e4f2864b5c372de3f7de0731cb7cd55ca5287b75a
else
$(package)_file_name=clang+llvm-$($(package)_version)-x86_64-linux-gnu-ubuntu-22.04.tar.xz
$(package)_sha256_hash=884ee67d647d77e58740c1e645649e29ae9e8a6fe87c1376be0f3a30f3cc9ab3
endif

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/lib/clang/$($(package)_major_version)/include && \
  mkdir -p $($(package)_staging_prefix_dir)/bin && \
  mkdir -p $($(package)_staging_prefix_dir)/include/llvm-c && \
  cp bin/clang $($(package)_staging_prefix_dir)/bin/ && \
  cp -P bin/clang++ $($(package)_staging_prefix_dir)/bin/ && \
  cp bin/dsymutil $($(package)_staging_prefix_dir)/bin/$(host)-dsymutil && \
  cp bin/llvm-config $($(package)_staging_prefix_dir)/bin/ && \
  cp bin/llvm-objdump $($(package)_staging_prefix_dir)/bin/$(host)-objdump && \
  cp include/llvm-c/ExternC.h $($(package)_staging_prefix_dir)/include/llvm-c && \
  cp include/llvm-c/lto.h $($(package)_staging_prefix_dir)/include/llvm-c && \
  cp lib/libLTO.so $($(package)_staging_prefix_dir)/lib/ && \
  cp -r lib/clang/$($(package)_major_version)/include/* $($(package)_staging_prefix_dir)/lib/clang/$($(package)_major_version)/include/
endef
