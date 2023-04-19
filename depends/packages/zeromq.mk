package=zeromq
$(package)_version=4.3.5
$(package)_download_path=https://github.com/zeromq/libzmq/releases/download/v$($(package)_version)/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=6653ef5910f17954861fe72332e68b03ca6e4d9c7160eb3a8de5a5a913bfab43
$(package)_patches=remove_libstd_link.patch fix_cmake_build.patch fix_have_windows_usage.patch
$(package)_build_subdir=build

define $(package)_set_vars
  $(package)_config_opts := -DZMQ_BUILD_TESTS=OFF -DWITH_DOCS=OFF -DWITH_LIBSODIUM=OFF
  $(package)_config_opts += -DWITH_LIBBSD=OFF -DENABLE_CURVE=OFF -DENABLE_CPACK=OFF
  $(package)_config_opts += -DBUILD_SHARED=OFF -DBUILD_TESTS=OFF -DZMQ_BUILD_TESTS=OFF
  $(package)_config_opts += -DENABLE_DRAFTS=OFF
  $(package)_config_opts_mingw32 += -DZMQ_WIN32_WINNT=0x0601 -DZMQ_HAVE_IPC=OFF
  $(package)_config_opts_debug := -DWITH_PERF_TOOL=OFF
endef

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/remove_libstd_link.patch && \
  patch -p1 < $($(package)_patch_dir)/fix_cmake_build.patch && \
  patch -p1 < $($(package)_patch_dir)/fix_have_windows_usage.patch
endef

define $(package)_config_cmds
   $($(package)_cmake) -S .. -B .
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm -rf bin share lib/*.la
endef
