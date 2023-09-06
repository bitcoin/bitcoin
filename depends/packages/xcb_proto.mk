package=xcb_proto
$(package)_version=1.15.2
$(package)_download_path=https://xorg.freedesktop.org/archive/individual/proto
$(package)_file_name=xcb-proto-$($(package)_version).tar.xz
$(package)_sha256_hash=7072beb1f680a2fe3f9e535b797c146d22528990c72f63ddb49d2f350a3653ed

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
  rm -rf lib/python*/site-packages/xcbgen/__pycache__
endef
