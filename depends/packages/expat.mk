package=expat
$(package)_version=2.2.5
$(package)_download_path=https://github.com/libexpat/libexpat/releases/download/R_2_2_5/
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=d9dc32efba7e74f788fcc4f212a43216fc37cf5f23f4c2339664d473353aedf6

define $(package)_set_vars
$(package)_config_opts=--disable-static
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
