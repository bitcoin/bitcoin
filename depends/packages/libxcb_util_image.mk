package=libxcb_util_image
$(package)_version=0.4.1
$(package)_download_path=https://xcb.freedesktop.org/dist
$(package)_file_name=xcb-util-image-$($(package)_version).tar.gz
$(package)_sha256_hash=0ebd4cf809043fdeb4f980d58cdcf2b527035018924f8c14da76d1c81001293b
$(package)_dependencies=libxcb libxcb_util

define $(package)_set_vars
$(package)_config_opts=--disable-shared --disable-devel-docs --without-doxygen
$(package)_config_opts+= --disable-dependency-tracking --enable-option-checking
endef

define $(package)_preprocess_cmds
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub .
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE) -C image
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) -C image install
endef

define $(package)_postprocess_cmds
  rm -rf share/man share/doc lib/*.la
endef
