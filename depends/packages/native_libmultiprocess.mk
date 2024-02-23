package=native_libmultiprocess
$(package)_version=5741d750a04e644a03336090d8979c6d033e32c0
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=ac848db49a6ed53e423c62d54bd87f1f08cbb0326254a8667e10bbfe5bf032a4
$(package)_dependencies=native_capnp

define $(package)_config_cmds
  $($(package)_cmake) .
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
