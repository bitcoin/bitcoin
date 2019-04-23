package=expat
$(package)_version=2.2.6
$(package)_download_path=https://github.com/libexpat/libexpat/releases/download/R_2_2_6/
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=17b43c2716d521369f82fc2dc70f359860e90fa440bea65b3b85f0b246ea81f2

define $(package)_set_vars
$(package)_config_opts=--disable-static --without-docbook
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
