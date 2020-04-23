package=altintegration
$(package)_version=55945a121b9ca49becb980f8a7433b36585c3edd
$(package)_download_path=https://github.com/VeriBlock/alt-integration-cpp/archive/
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=2eb173a47836c28c14a28efd8305b9c4364a3c26509229d2e125e0919322e5a7

ifeq ($(HOST), x86_64-w64-mingw32)
  define $(package)_config_cmds
    cmake -DCMAKE_C_COMPILER=$(HOST)-gcc -DCMAKE_CXX_COMPILER=$(HOST)-g++ -DCMAKE_INSTALL_PREFIX=$($(package)_staging_dir)$(host_prefix) -DCMAKE_BUILD_TYPE=Release -DTESTING=OFF -DWITH_ROCKSDB=OFF -DSHARED=OFF -B .
  endef
else
  define $(package)_config_cmds
    cmake -DCMAKE_INSTALL_PREFIX=$($(package)_staging_dir)$(host_prefix) -DCMAKE_BUILD_TYPE=Release -DTESTING=OFF -DWITH_ROCKSDB=OFF -DSHARED=OFF -B .
  endef
endif

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) install
endef