package=xcb_proto
$(package)_version=1.14
$(package)_download_path=https://xcb.freedesktop.org/dist
$(package)_file_name=xcb-proto-$($(package)_version).tar.xz
$(package)_sha256_hash=186a3ceb26f9b4a015f5a44dcc814c93033a5fc39684f36f1ecc79834416a605

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
  find -name "*.pyc" -delete && \
  find -name "*.pyo" -delete
endef
