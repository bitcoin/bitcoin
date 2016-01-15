package=univalue
$(package)_version=1.0.1
$(package)_download_path=https://codeload.github.com/jgarzik/$(package)/tar.gz
$(package)_file_name=v$($(package)_version)
$(package)_sha256_hash=1699d5dcaabc29dde024dbfd42cb49915d4c135dfbbb5cc846de0654ca30cdab

define $(package)_preprocess_cmds
  cd $($(package)_build_subdir); ./autogen.sh
endef

define $(package)_set_vars
$(package)_config_opts=--disable-shared --with-pic
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
