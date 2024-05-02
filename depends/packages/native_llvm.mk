package=native_llvm
$(package)_version=18.1.4
$(package)_major_version=$(firstword $(subst ., ,$($(package)_version)))
$(package)_download_path=https://github.com/llvm/llvm-project/releases/download/llvmorg-$($(package)_version)
ifneq (,$(findstring aarch64,$(BUILD)))
$(package)_file_name=clang+llvm-$($(package)_version)-aarch64-linux-gnu.tar.xz
$(package)_sha256_hash=8c2f4d1606d24dc197a590acce39453abe7a302b9b92e762108f9b5a9701b1df
else
$(package)_file_name=clang+llvm-$($(package)_version)-x86_64-linux-gnu-ubuntu-18.04.tar.xz
$(package)_sha256_hash=1607375b4aa2aec490b6db51846a04b265675a87e925bcf5825966401ff9b0b1
endif

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/lib/clang/$($(package)_major_version)/include && \
  mkdir -p $($(package)_staging_prefix_dir)/bin && \
  mkdir -p $($(package)_staging_prefix_dir)/include/llvm-c && \
  cp bin/clang $($(package)_staging_prefix_dir)/bin/ && \
  cp -P bin/clang++ $($(package)_staging_prefix_dir)/bin/ && \
  cp bin/dsymutil $($(package)_staging_prefix_dir)/bin/dsymutil && \
  cp bin/ld64.lld $($(package)_staging_prefix_dir)/bin/ld64.lld && \
  cp bin/llvm-ar $($(package)_staging_prefix_dir)/bin/llvm-ar && \
  cp bin/llvm-config $($(package)_staging_prefix_dir)/bin/ && \
  cp bin/llvm-nm $($(package)_staging_prefix_dir)/bin/llvm-nm && \
  cp bin/llvm-objdump $($(package)_staging_prefix_dir)/bin/llvm-objdump && \
  cp bin/llvm-ranlib $($(package)_staging_prefix_dir)/bin/llvm-ranlib && \
  cp bin/llvm-strip $($(package)_staging_prefix_dir)/bin/llvm-strip && \
  cp include/llvm-c/ExternC.h $($(package)_staging_prefix_dir)/include/llvm-c && \
  cp include/llvm-c/lto.h $($(package)_staging_prefix_dir)/include/llvm-c && \
  cp lib/libLTO.so $($(package)_staging_prefix_dir)/lib/ && \
  cp -r lib/clang/$($(package)_major_version)/include/* $($(package)_staging_prefix_dir)/lib/clang/$($(package)_major_version)/include/
endef
