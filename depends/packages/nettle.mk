package=nettle
$(package)_version=3.3
$(package)_download_path=https://ftp.gnu.org/gnu/nettle
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=46942627d5d0ca11720fec18d81fc38f7ef837ea4197c1f630e71ce0d470b11e
# default settings  $(package)_config_opts_default=--enable-pic -fPIC -static

# set settings based on host
$(package)_config_opts = $(if $($(package)_config_opts_$(HOST)), $($(package)_config_opts_$(HOST)), $($(package)_config_opts_default))

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef