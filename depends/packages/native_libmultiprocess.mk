package=native_libmultiprocess
$(package)_version=07c917f7ca910d66abc6d3873162fc9061704074
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=ac9db311e3b22aac3c7b7b7b3f6b7fee5cf3043ebb3c3bf412049e8b17166de8
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
