package=nettle
$(package)_version=3.7.1
$(package)_dependencies=gmp
$(package)_download_path=https://ftp.gnu.org/gnu/nettle
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=156621427c7b00a75ff9b34b770b95d34f80ef7a55c3407de94b16cbf436c42e

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