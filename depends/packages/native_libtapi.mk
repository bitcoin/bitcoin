package=native_libtapi
$(package)_version=3efb201881e7a76a21e0554906cf306432539cef
$(package)_download_path=https://github.com/tpoechtrager/apple-libtapi/archive
$(package)_download_file=$($(package)_version).tar.gz
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=380c1ca37cfa04a8699d0887a8d3ee1ad27f3d08baba78887c73b09485c0fbd3

ifeq ($(strip $(FORCE_USE_SYSTEM_CLANG)),)
$(package)_dependencies=native_clang
endif

define $(package)_build_cmds
  CC=$(clang_prog) CXX=$(clangxx_prog) INSTALLPREFIX=$($(package)_staging_prefix_dir) ./build.sh
endef

define $(package)_stage_cmds
  ./install.sh && \
  mkdir -p $($(package)_staging_prefix_dir)/include/llvm-c && \
  cp src/llvm/include/llvm-c/lto.h $($(package)_staging_prefix_dir)/include/llvm-c
endef
