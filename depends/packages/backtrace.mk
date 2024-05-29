package=backtrace
$(package)_version=11427f31a64b11583fec94b4c2a265c7dafb1ab3
$(package)_download_path=https://github.com/ianlancetaylor/libbacktrace/archive
$(package)_download_file=$($(package)_version).tar.gz
$(package)_file_name=$(package)_$($(package)_version).tar.gz
$(package)_sha256_hash=76a8348ff04d80141aeb1c0e55879f17f241f38238def0eb1df7c6d1ac1a2c26

define $(package)_set_vars
$(package)_config_opts=--disable-shared --enable-host-shared --prefix=$(host_prefix)
endef

define $(package)_config_cmds
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub . && \
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm lib/*.la
endef
