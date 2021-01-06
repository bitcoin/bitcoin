package=gmp
$(package)_version=6.2.0
$(package)_download_path=https://gmplib.org/download/gmp
$(package)_file_name=gmp-$($(package)_version).tar.xz
$(package)_sha256_hash=258e6cd51b3fbdfc185c716d55f82c08aff57df0c6fbd143cf6ed561267a1526

define $(package)_set_vars
$(package)_config_opts+=--enable-cxx --enable-fat --disable-shared --enable-static
$(package)_config_opts_linux=--with-pic
$(package)_config_opts_darwin=--with-pic
$(package)_cflags_armv7l_linux+=-march=armv7-a
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
  rm lib/*.la
endef