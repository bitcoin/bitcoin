package=altintegration
$(package)_version=a313049c3239a92ca07d95d4dc55a0a37fb3555b
$(package)_download_path=https://github.com/VeriBlock/alt-integration-cpp/archive/
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=3cef4388f26278dc89e4a2958f040fbc1655e53c6c3f393b45bd1983699a6cf7

define $(package)_config_cmds
  cmake -DCMAKE_C_COMPILER=$(CC) -DCMAKE_CXX_COMPILER=$(CXX) -DCMAKE_INSTALL_PREFIX=$($(package)_staging_dir)$(host_prefix) -DCMAKE_BUILD_TYPE=Release -DTESTING=OFF -DWITH_ROCKSDB=OFF -DSHARED=OFF -B .
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) install
endef