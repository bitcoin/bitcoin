package=miniupnpc
$(package)_version=2.2.7
$(package)_download_path=https://miniupnp.tuxfamily.org/files/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=b0c3a27056840fd0ec9328a5a9bac3dc5e0ec6d2e8733349cf577b0aa1e70ac1
$(package)_patches=dont_leak_info.patch cmake_get_src_addr.patch fix_windows_snprintf.patch
$(package)_build_subdir=build

define $(package)_set_vars
$(package)_config_opts = -DUPNPC_BUILD_SAMPLE=OFF -DUPNPC_BUILD_SHARED=OFF
$(package)_config_opts += -DUPNPC_BUILD_STATIC=ON -DUPNPC_BUILD_TESTS=OFF
$(package)_config_opts_mingw32 += -DMINIUPNPC_TARGET_WINDOWS_VERSION=0x0601
endef

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/dont_leak_info.patch && \
  patch -p1 < $($(package)_patch_dir)/cmake_get_src_addr.patch && \
  patch -p1 < $($(package)_patch_dir)/fix_windows_snprintf.patch
endef

define $(package)_config_cmds
  $($(package)_cmake) -S .. -B .
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  cmake --install . --prefix $($(package)_staging_prefix_dir)
endef
