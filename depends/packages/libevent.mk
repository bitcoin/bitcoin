package=libevent
$(package)_version=2.0.22
$(package)_download_path=https://github.com/libevent/libevent/releases/download/release-2.0.22-stable
$(package)_file_name=$(package)-$($(package)_version)-stable.tar.gz
$(package)_sha256_hash=71c2c49f0adadacfdbe6332a372c38cf9c8b7895bb73dabeaa53cdcc1d4e1fa3
$(package)_patches=reuseaddr.patch libevent-2-fixes.patch

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/reuseaddr.patch && \
  patch -p1 < $($(package)_patch_dir)/libevent-2-fixes.patch
endef

define $(package)_set_vars
  $(package)_config_opts=--disable-shared --disable-openssl --disable-libevent-regress
  $(package)_config_opts_release=--disable-debug-mode
  $(package)_config_opts_linux=--with-pic
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
endef
