package=native_libmultiprocess
$(package)_version=abe254b9734f2e2b220d1456de195532d6e6ac1e
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=85777073259fdc75d24ac5777a19991ec1156c5f12db50b252b861c95dcb4f46
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
