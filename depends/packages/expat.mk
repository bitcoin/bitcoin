package=expat
$(package)_version=2.2.1
$(package)_download_path=https://github.com/libexpat/libexpat/releases/download/R_2_2_1/
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=1868cadae4c82a018e361e2b2091de103cd820aaacb0d6cfa49bd2cd83978885

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
