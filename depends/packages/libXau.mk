package=libXau
$(package)_version=1.0.12
$(package)_download_path=https://xorg.freedesktop.org/releases/individual/lib/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=2402dd938da4d0a332349ab3d3586606175e19cb32cb9fe013c19f1dc922dcee
$(package)_dependencies=xproto

# When updating this package, check the default value of
# --disable-xthreads. It is currently enabled.
define $(package)_set_vars
  $(package)_config_opts=--disable-shared --disable-lint-library --without-lint
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
  rm -rf share lib/*.la
endef
