package=nettle
$(package)_version=3.5
$(package)_dependencies=gmp
$(package)_download_path=https://ftp.gnu.org/gnu/nettle
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=432d98dcd341cbd063a963025524122368c1f2ee9c721bb68d771d5e34674b4f

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