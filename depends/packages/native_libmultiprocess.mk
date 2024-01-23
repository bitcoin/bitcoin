package=native_libmultiprocess
$(package)_version=8da797c5f1644df1bffd84d10c1ae9836dc70d60
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=030f4d393d2ac9deba98d2e1973e22fc439ffc009d5f8ae3225c90639f86beb0
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
