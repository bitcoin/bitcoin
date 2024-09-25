package=native_libmultiprocess
$(package)_version=015e95f7ebaa47619a213a19801e7fffafc56864
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=4b1266b121337f3f6f37e1863fba91c1a5ee9ad126bcffc6fe6b9ca47ad050a1
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
