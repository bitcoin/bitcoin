package=native_libmultiprocess
$(package)_version=1954f7f65661d49e700c344eae0fc8092decf975
$(package)_download_path=https://github.com/bitcoin-core/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=fc014bd74727c1d5d30b396813685012c965d079244dd07b53bc1c75c610a2cb
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
