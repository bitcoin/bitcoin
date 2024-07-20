package=native_libmultiprocess
$(package)_version=8b8a4766ce0a1892b9e8a5eb73dc39821005e520
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=475c0dc2357a2ff30e9a164e4c16dc8a6597a57c9193d646fa9cbf0a55c45d78
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
