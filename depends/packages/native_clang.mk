package=native_clang
$(package)_version=8.0.0
$(package)_download_path=https://releases.llvm.org/$($(package)_version)
$(package)_download_file=clang+llvm-$($(package)_version)-x86_64-linux-gnu-ubuntu-14.04.tar.xz
$(package)_file_name=clang-llvm-$($(package)_version)-x86_64-linux-gnu-ubuntu-14.04.tar.xz
$(package)_sha256_hash=9ef854b71949f825362a119bf2597f744836cb571131ae6b721cd102ffea8cd0

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
