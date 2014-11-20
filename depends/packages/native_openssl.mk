package=native_openssl
$(package)_version=1.0.1h
$(package)_download_path=https://www.openssl.org/source
$(package)_file_name=openssl-$($(package)_version).tar.gz
$(package)_sha256_hash=9d1c8a9836aa63e2c6adb684186cbd4371c9e9dcc01d6e3bb447abf2d4d3d093
define $(package)_set_vars
$(package)_build_config_opts= --prefix=$(build_prefix) no-zlib no-shared no-krb5C linux-generic32 -m32
endef

define $(package)_config_cmds
  ./Configure $($(package)_build_config_opts) &&\
  sed -i "s|engines apps test|engines|" Makefile
endef

define $(package)_build_cmds
  $(MAKE) -j1
endef

define $(package)_stage_cmds
  $(MAKE) INSTALL_PREFIX=$($(package)_staging_dir) -j1 install_sw
endef
