package=libxcb_util_image
$(package)_version=0.4.0
$(package)_download_path=https://xcb.freedesktop.org/dist
$(package)_file_name=xcb-util-image-$($(package)_version).tar.bz2
$(package)_sha256_hash=2db96a37d78831d643538dd1b595d7d712e04bdccf8896a5e18ce0f398ea2ffc
$(package)_dependencies=libxcb libxcb_util
$(package)_patches=missing_xcb_util_pkg_config.patch

define $(package)_set_vars
$(package)_config_opts=--disable-static --disable-devel-docs --without-doxygen
$(package)_config_opts+= --disable-dependency-tracking --enable-option-checking
endef

define $(package)_preprocess_cmds
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub . && \
  patch -p1 < $($(package)_patch_dir)/missing_xcb_util_pkg_config.patch
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
