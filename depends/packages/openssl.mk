package=openssl
$(package)_version=1.0.1j
$(package)_download_path=https://www.openssl.org/source
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=1b60ca8789ba6f03e8ef20da2293b8dc131c39d83814e775069f02d26354edf3

define $(package)_set_vars
$(package)_config_env=AR="$($(package)_ar)" RANLIB="$($(package)_ranlib)" CC="$($(package)_cc)"
$(package)_config_opts=--prefix=$(host_prefix) --openssldir=$(host_prefix)/etc/openssl no-zlib no-shared no-dso
$(package)_config_opts+=no-krb5 no-camellia no-capieng no-cast no-cms no-dtls1 no-gost no-gmp no-heartbeats no-idea no-jpake no-md2
$(package)_config_opts+=no-mdc2 no-rc5 no-rdrand no-rfc3779 no-rsax no-sctp no-seed no-sha0 no-static_engine no-whirlpool no-rc2 no-rc4 no-ssl2 no-ssl3
$(package)_config_opts+=$($(package)_cflags) $($(package)_cppflags)
$(package)_config_opts_x86_64_linux=-fPIC linux-x86_64
$(package)_config_opts_arm_linux=-fPIC linux-generic32
$(package)_config_opts_x86_64_darwin=darwin64-x86_64-cc
$(package)_config_opts_x86_64_mingw32=mingw64
$(package)_config_opts_i686_mingw32=mingw
$(package)_config_opts_i686_linux=linux-generic32 -fPIC
endef

define $(package)_preprocess_cmds
  sed -i.old "/define DATE/d" crypto/Makefile && \
  sed -i.old "s|engines apps test|engines|" Makefile.org
endef

define $(package)_config_cmds
  ./Configure $($(package)_config_opts)
endef

define $(package)_build_cmds
  $(MAKE) -j1 build_libs libcrypto.pc libssl.pc openssl.pc
endef

define $(package)_stage_cmds
  $(MAKE) INSTALL_PREFIX=$($(package)_staging_dir) -j1 install_sw
endef

define $(package)_postprocess_cmds
  rm -rf share bin etc
endef
