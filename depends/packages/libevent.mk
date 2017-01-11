package=libevent
$(package)_version=2.1.7
$(package)_download_path=https://github.com/libevent/libevent/archive/
$(package)_file_name=release-$($(package)_version)-rc.tar.gz
$(package)_sha256_hash=548362d202e22fe24d4c3fad38287b4f6d683e6c21503341373b89785fa6f991

define $(package)_preprocess_cmds
  ./autogen.sh
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
