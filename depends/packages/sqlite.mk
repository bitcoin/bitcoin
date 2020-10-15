package=sqlite
$(package)_version=3320100
$(package)_download_path=https://sqlite.org/2020/
$(package)_file_name=sqlite-autoconf-$($(package)_version).tar.gz
$(package)_sha256_hash=486748abfb16abd8af664e3a5f03b228e5f124682b0c942e157644bf6fff7d10

define $(package)_set_vars
$(package)_config_opts=--disable-shared --disable-readline --disable-dynamic-extensions --enable-option-checking
$(package)_config_opts_linux=--with-pic
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
