package=zlib
$(package)_version=1.2.11
$(package)_download_path=https://www.zlib.net
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1

define $(package)_set_vars
$(package)_config_opts= CC="$($(package)_cc)"
$(package)_config_opts+=CFLAGS="$($(package)_cflags) $($(package)_cppflags) -fPIC"
$(package)_config_opts+=RANLIB="$($(package)_ranlib)"
$(package)_config_opts+=AR="$($(package)_ar)"
$(package)_config_opts_darwin+=AR="$($(package)_libtool)"
$(package)_config_opts_darwin+=ARFLAGS="-o"
$(package)_config_opts_android+=CHOST=$(host)
endef

# zlib has its own custom configure script that takes in options like CC,
# CFLAGS, RANLIB, AR, and ARFLAGS from the environment rather than from
# command-line arguments.
define $(package)_config_cmds
  env $($(package)_config_opts) ./configure --static --prefix=$(host_prefix)
endef

define $(package)_build_cmds
  $(MAKE) libz.a
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef