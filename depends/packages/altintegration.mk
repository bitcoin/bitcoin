package=altintegration
$(package)_version=82c2cffe4cd87343a6a256934fd0694d2071a5c6
$(package)_download_path=https://github.com/VeriBlock/alt-integration-cpp/archive/
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=820d689e4657cc248881931dc017bd06c75812535c090c9c396ea3d02a24d885

define $(package)_config_cmds
  cmake -DCMAKE_INSTALL_PREFIX=$($(package)_staging_dir)$(host_prefix) -DCMAKE_BUILD_TYPE=Release -DTESTING=OFF -DWITH_ROCKSDB=OFF -DSHARED=OFF -B .
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) install
endef


