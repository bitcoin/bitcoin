package=libxcb_util_cursor
$(package)_version=0.1.5
$(package)_download_path=https://xcb.freedesktop.org/dist
$(package)_file_name=xcb-util-cursor-$($(package)_version).tar.gz
$(package)_sha256_hash=0e9c5446dc6f3beb8af6ebfcc9e27bcc6da6fe2860f7fc07b99144dfa568e93b
$(package)_dependencies=libxcb libxcb_util_render libxcb_util_image

define $(package)_set_vars
$(package)_config_opts = --disable-static
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
  rm lib/*.la
endef
