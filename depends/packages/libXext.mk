package=libXext
$(package)_version=1.3.3
$(package)_download_path=https://xorg.freedesktop.org/releases/individual/lib/
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=b518d4d332231f313371fdefac59e3776f4f0823bcb23cf7c7305bfb57b16e35
$(package)_dependencies=xproto xextproto libX11 libXau

define $(package)_set_vars
  $(package)_config_opts=--disable-shared
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
