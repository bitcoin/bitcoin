package=univalue
$(package)_version=1.0.2
$(package)_download_path=https://codeload.github.com/jgarzik/$(package)/tar.gz
$(package)_file_name=v$($(package)_version)
$(package)_sha256_hash=685ca5d2db9c0475d88bfd0a444a90ade770f7e98dacfed55921775c36d28e51

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
