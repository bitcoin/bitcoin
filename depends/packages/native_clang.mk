package=native_clang
$(package)_version=11.1.0
$(package)_download_path=https://github.com/llvm/llvm-project/releases/download/llvmorg-$($(package)_version)
ifneq (,$(findstring aarch64,$(BUILD)))
$(package)_file_name=clang+llvm-$($(package)_version)-aarch64-linux-gnu.tar.xz
$(package)_sha256_hash=18df38247af3fba0e0e2991fb00d7e3cf3560b4d3509233a14af699ef0039e1c
else
$(package)_file_name=clang+llvm-$($(package)_version)-x86_64-linux-gnu-ubuntu-16.04.tar.xz
$(package)_sha256_hash=c691a558967fb7709fb81e0ed80d1f775f4502810236aa968b4406526b43bee1
endif

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/lib/clang/$($(package)_version)/include && \
  mkdir -p $($(package)_staging_prefix_dir)/bin && \
  mkdir -p $($(package)_staging_prefix_dir)/include/llvm-c && \
  cp bin/clang $($(package)_staging_prefix_dir)/bin/ && \
  cp -P bin/clang++ $($(package)_staging_prefix_dir)/bin/ && \
  cp bin/dsymutil $($(package)_staging_prefix_dir)/bin/$(host)-dsymutil && \
  cp bin/llvm-config $($(package)_staging_prefix_dir)/bin/ && \
  cp include/llvm-c/ExternC.h $($(package)_staging_prefix_dir)/include/llvm-c && \
  cp include/llvm-c/lto.h $($(package)_staging_prefix_dir)/include/llvm-c && \
  cp lib/libLTO.so $($(package)_staging_prefix_dir)/lib/ && \
  cp -r lib/clang/$($(package)_version)/include/* $($(package)_staging_prefix_dir)/lib/clang/$($(package)_version)/include/
endef
