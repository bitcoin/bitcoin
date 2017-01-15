package=xextproto
$(package)_version=7.3.0
$(package)_download_path=http://xorg.freedesktop.org/releases/individual/proto
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=f3f4b23ac8db9c3a9e0d8edb591713f3d70ef9c3b175970dd8823dfc92aa5bb0

define $(package)_set_vars
$(package)_config_opts=--disable-shared
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
