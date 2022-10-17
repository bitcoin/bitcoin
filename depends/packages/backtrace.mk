package=backtrace
$(package)_version=rust-snapshot-2018-05-22
$(package)_download_path=https://github.com/rust-lang-nursery/libbacktrace/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=8da6daa0a582c9bbd1f2933501168b4c43664700f604f43e922e85b99e5049bc

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
