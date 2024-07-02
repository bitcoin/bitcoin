package=miniupnpc
$(package)_version=2.2.8
$(package)_download_path=http://miniupnp.free.fr/files/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=05b929679091b9921b6b6c1f25e39e4c8d1f4d46c8feb55a412aa697aee03a93
$(package)_patches=dont_leak_info.patch
$(package)_build_subdir=build

define $(package)_set_vars
$(package)_config_opts = -DUPNPC_BUILD_SAMPLE=OFF -DUPNPC_BUILD_SHARED=OFF
$(package)_config_opts += -DUPNPC_BUILD_STATIC=ON -DUPNPC_BUILD_TESTS=OFF
$(package)_config_opts_mingw32 += -DMINIUPNPC_TARGET_WINDOWS_VERSION=0x0601
endef

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/dont_leak_info.patch
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
