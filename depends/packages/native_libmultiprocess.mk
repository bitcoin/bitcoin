package=native_libmultiprocess
$(package)_version=c1b4ab4eb897d3af09bc9b3cc30e2e6fff87f3e2
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=6edf5ad239ca9963c78f7878486fb41411efc9927c6073928a7d6edf947cac4a
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
