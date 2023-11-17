package=native_libmultiprocess
$(package)_version=414542f81e0997354b45b8ade13ca144a3e35ff1
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=8542dbaf8c4fce8fd7af6929f5dc9b34dffa51c43e9ee360e93ee0f34b180bc2
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
