package=native_clang
$(package)_version=10.0.1
$(package)_download_path=https://github.com/llvm/llvm-project/releases/download/llvmorg-$($(package)_version)
ifneq (,$(findstring aarch64,$(BUILD)))
$(package)_download_file=clang+llvm-$($(package)_version)-aarch64-linux-gnu.tar.xz
$(package)_file_name=clang+llvm-$($(package)_version)-aarch64-linux-gnu.tar.xz
$(package)_sha256_hash=90dc69a4758ca15cd0ffa45d07fbf5bf4309d47d2c7745a9f0735ecffde9c31f
else
$(package)_download_file=clang+llvm-$($(package)_version)-x86_64-linux-gnu-ubuntu-16.04.tar.xz
$(package)_file_name=clang+llvm-$($(package)_version)-x86_64-linux-gnu-ubuntu-16.04.tar.xz
$(package)_sha256_hash=48b83ef827ac2c213d5b64f5ad7ed082c8bcb712b46644e0dc5045c6f462c231
endif

define $(package)_preprocess_cmds
  rm -f $($(package)_extract_dir)/lib/libc++abi.so*
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/lib/clang/$($(package)_version)/include && \
  mkdir -p $($(package)_staging_prefix_dir)/bin && \
  mkdir -p $($(package)_staging_prefix_dir)/include && \
  cp bin/clang $($(package)_staging_prefix_dir)/bin/ && \
  cp -P bin/clang++ $($(package)_staging_prefix_dir)/bin/ && \
  cp bin/dsymutil $($(package)_staging_prefix_dir)/bin/$(host)-dsymutil && \
  cp bin/llvm-config $($(package)_staging_prefix_dir)/bin/ && \
  cp lib/libLTO.so $($(package)_staging_prefix_dir)/lib/ && \
  cp -rf lib/clang/$($(package)_version)/include/* $($(package)_staging_prefix_dir)/lib/clang/$($(package)_version)/include/
endef

define $(package)_postprocess_cmds
  rmdir include
endef
