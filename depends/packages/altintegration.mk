package=altintegration
$(package)_version=7362db2fb529157d39d4273f45b36e0fa0f1f7f7
$(package)_download_path=https://github.com/VeriBlock/alt-integration-cpp/archive/
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=567d323d32fb88037469f404502fbe525d1ba6cced4ab391d61ac4baa358178a

define $(package)_config_cmds
  cmake -DCMAKE_INSTALL_PREFIX=$($(package)_staging_dir)$(host_prefix) -DCMAKE_BUILD_TYPE=Release -DTESTING=OFF -DWITH_ROCKSDB=OFF -DSHARED=OFF -B .
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) install
endef


