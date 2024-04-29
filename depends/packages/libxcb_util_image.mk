package=libxcb_util_image
$(package)_version=0.4.0
$(package)_download_path=https://xcb.freedesktop.org/dist
$(package)_file_name=xcb-util-image-$($(package)_version).tar.gz
$(package)_sha256_hash=cb2c86190cf6216260b7357a57d9100811bb6f78c24576a3a5bfef6ad3740a42
$(package)_dependencies=libxcb libxcb_util

define $(package)_set_vars
$(package)_config_opts=--disable-static --disable-devel-docs --without-doxygen
$(package)_config_opts+= --disable-dependency-tracking --enable-option-checking
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
