package=nettle
$(package)_version=3.6
$(package)_dependencies=gmp
$(package)_download_path=https://ftp.gnu.org/gnu/nettle
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=d24c0d0f2abffbc8f4f34dcf114b0f131ec3774895f3555922fe2f40f3d5e3f1

# default settings
$(package)_config_opts=--libdir=$(host_prefix)/lib --disable-shared --enable-static --disable-documentation --disable-openssl
$(package)_config_opts_linux=--enable-pic

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef