package=libxcb_util
$(package)_version=0.4.0
$(package)_download_path=https://xcb.freedesktop.org/dist
$(package)_file_name=xcb-util-$($(package)_version).tar.bz2
$(package)_sha256_hash=46e49469cb3b594af1d33176cd7565def2be3fa8be4371d62271fabb5eae50e9
$(package)_dependencies=libxcb

define $(package)_set_vars
$(package)_config_opts = --disable-shared --disable-devel-docs --without-doxygen
$(package)_config_opts += --disable-dependency-tracking --enable-option-checking
$(package)_config_opts += --with-pic
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

define $(package)_postprocess_cmds
  rm -rf share/man share/doc lib/*.la
endef
