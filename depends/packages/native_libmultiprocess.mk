package=native_libmultiprocess
$(package)_version=2350843a3196ee2bf1253a8061aaf2d4b1ea462b
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=084471ae7df3b10fa254fbe83f12835d6dda6816c15eba530efcea65d975f5a5
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
