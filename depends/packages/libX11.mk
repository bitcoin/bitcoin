package=libX11
$(package)_version=1.6.2
$(package)_download_path=https://xorg.freedesktop.org/releases/individual/lib/
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=2aa027e837231d2eeea90f3a4afe19948a6eb4c8b2bec0241eba7dbc8106bd16
$(package)_patches=configure.ac

define $(package)_preprocess_cmds
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub . && \
  cp -f $($(package)_patch_dir)/configure.ac .
endef

define $(package)_config_cmds
  autoreconf --install --force --verbose && \
  $($(package)_autoconf)
endef

define $(package)_stage_cmds
  $(MAKE) -C include DESTDIR=$($(package)_staging_dir) install-x11includeHEADERS
endef
