package:=native_libuuid
$(package)_version=1.41.14
$(package)_download_path=http://downloads.sourceforge.net/e2fsprogs
$(package)_file_name=e2fsprogs-libs-$($(package)_version).tar.gz
$(package)_sha256_hash=dbc7a138a3218d9b80a0626b5b692d76934d6746d8cbb762751be33785d8d9f5

define $(package)_set_vars
$(package)_config_opts=--disable-elf-shlibs --disable-uuidd
$(package)_cflags+=-m32
$(package)_ldflags+=-m32
$(package)_cxxflags+=-m32
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE) -C lib/uuid
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) -C lib/uuid install
endef
