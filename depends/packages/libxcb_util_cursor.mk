package=libxcb_util_cursor
$(package)_version=0.1.4
$(package)_download_path=https://xcb.freedesktop.org/dist
$(package)_file_name=xcb-util-cursor-$($(package)_version).tar.gz
$(package)_sha256_hash=28dcfe90bcab7b3561abe0dd58eb6832aa9cc77b723753e1cb42dde3f023f057
$(package)_dependencies=libxcb libxcb_util_render libxcb_util_image

define $(package)_set_vars
$(package)_config_opts = --disable-shared --disable-devel-docs --without-doxygen
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

define $(package)_postprocess_cmds
  rm -rf share/man share/doc lib/*.la
endef
