package=native_zlib
$(package)_version=1.2.11
$(package)_download_path=http://www.zlib.net
$(package)_file_name=zlib-$($(package)_version).tar.gz
$(package)_sha256_hash=c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1

define $(package)_config_cmds
  ./configure --prefix=$(build_prefix)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

