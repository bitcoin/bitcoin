package=xproto
$(package)_version=7.0.31
$(package)_download_path=https://xorg.freedesktop.org/releases/individual/proto
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=6d755eaae27b45c5cc75529a12855fed5de5969b367ed05003944cf901ed43c7

define $(package)_set_vars
$(package)_config_opts=--without-fop --without-xmlto --without-xsltproc --disable-specs
$(package)_config_opts += --disable-dependency-tracking --enable-option-checking
endef

define $(package)_preprocess_cmds
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub .
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
