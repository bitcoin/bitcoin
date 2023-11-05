package=native_libmultiprocess
$(package)_version=61d5a0e661f20a4928fbf868ec9c3c6f17883cc7
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=5cfda224cc2ce913f2493f843317e0fca3184f6d7c1434c9754b2e7dca440ab5
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
