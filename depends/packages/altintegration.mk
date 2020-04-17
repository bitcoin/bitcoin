package=altintegration
$(package)_version=c32afd4ed526b14631a4764ae5897e2e461cb4fe
$(package)_download_path=https://github.com/VeriBlock/alt-integration-cpp/archive/
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=543128f8483f8a6a4bb78f203e37989151463318d8de81cad3fb08ce1cb45d03

define $(package)_config_cmds
  cmake -DCMAKE_INSTALL_PREFIX=$($(package)_staging_dir)$(host_prefix) -DCMAKE_BUILD_TYPE=Release -DTESTING=OFF -DWITH_ROCKSDB=OFF -DSHARED=OFF -B .
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) install
endef


