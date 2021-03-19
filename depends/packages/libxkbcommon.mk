package=libxkbcommon
$(package)_version=0.8.4
$(package)_download_path=https://xkbcommon.org/download/
$(package)_file_name=$(package)-$($(package)_version).tar.xz
$(package)_sha256_hash=60ddcff932b7fd352752d51a5c4f04f3d0403230a584df9a2e0d5ed87c486c8b
$(package)_dependencies=libxcb

define $(package)_set_vars
$(package)_config_opts = --enable-option-checking --disable-dependency-tracking
$(package)_config_opts += --disable-static --disable-docs
endef

define $(package)_preprocess_cmds
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub build-aux
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

