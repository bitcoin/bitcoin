package=sqlite
$(package)_version=3380500
$(package)_download_path=https://sqlite.org/2022/
$(package)_file_name=sqlite-autoconf-$($(package)_version).tar.gz
$(package)_sha256_hash=5af07de982ba658fd91a03170c945f99c971f6955bc79df3266544373e39869c

define $(package)_set_vars
$(package)_config_opts=--disable-shared --disable-readline --disable-dynamic-extensions --enable-option-checking
$(package)_config_opts_linux=--with-pic
$(package)_cppflags_linux = -DSQLITE_DISABLE_LFS
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE) libsqlite3.la
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install-libLTLIBRARIES install-includeHEADERS install-pkgconfigDATA
endef

define $(package)_postprocess_cmds
  rm lib/*.la
endef
