package=native_libmultiprocess
$(package)_version=003eb04d6d0029fd24a330ab63d5a9ba08cf240f
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=d23e82f7a0b498a876a4bcdecca3104032a9f9372e1a0cf0049409a2718e5d39
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
