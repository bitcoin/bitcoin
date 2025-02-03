package=native_libmultiprocess
$(package)_version=011fc53aeaf2b8bfceb8e738aa9bf512a240496e
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=43737480f59fbad16db33dd08790fd245bb6660491a9a41f060406c26d1a23d4
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
