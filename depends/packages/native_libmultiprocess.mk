package=native_libmultiprocess
$(package)_version=35944ffd23fa26652b82210351d50e896ce16c8f
$(package)_download_path=https://github.com/bitcoin-core/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=b542f270c076d0287c124e7f97b21ab7e32b979ff182491c157c2da9dec723c4
$(package)_dependencies=native_capnp

define $(package)_config_cmds
  $($(package)_cmake) .
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install-bin
endef
