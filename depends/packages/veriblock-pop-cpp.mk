package=veriblock-pop-cpp
$(package)_version=acc1e440de00aedeaf977ec4cdc49e627dc934fe
$(package)_download_path=https://github.com/VeriBlock/alt-integration-cpp/archive/
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=62e901619e0c40cd81bf5e9c68cc8e4486c2d72ed22114c984468b779b980245
$(package)_build_subdir=build
$(package)_build_type=$(BUILD_TYPE)
$(package)_asan=$(ASAN)

define $(package)_preprocess_cmds
  mkdir -p build
endef

ifeq ($(strip $(HOST)),)
  define $(package)_config_cmds
    cmake -DCMAKE_INSTALL_PREFIX=$(host_prefix) -DCMAKE_BUILD_TYPE=$(package)_build_type \
    -DTESTING=OFF -DSHARED=OFF -DASAN:BOOL=$(package)_asan ..
  endef
else ifeq ($(HOST), x86_64-apple-darwin16)
  define $(package)_config_cmds
    cmake -DCMAKE_C_COMPILER=$(darwin_CC) -DCMAKE_CXX_COMPILER=$(darwin_CXX) \
    -DCMAKE_INSTALL_PREFIX=$(host_prefix) -DCMAKE_BUILD_TYPE=$(package)_build_type \
    -DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_SYSTEM_NAME=Darwin -DCMAKE_SYSTEM_PROCESSOR=x86_64 \
    -DCMAKE_C_COMPILER_TARGET=$(HOST) -DCMAKE_CXX_COMPILER_TARGET=$(HOST) \
    -DCMAKE_OSX_SYSROOT=$(OSX_SDK) -DTESTING=OFF \
    -DSHARED=OFF ..
  endef
else ifeq ($(HOST), x86_64-pc-linux-gnu)
  define $(package)_config_cmds
    cmake -DCMAKE_INSTALL_PREFIX=$(host_prefix) -DCMAKE_BUILD_TYPE=$(package)_build_type \
    -DTESTING=OFF -DSHARED=OFF ..
  endef
else
  define $(package)_config_cmds
    cmake -DCMAKE_C_COMPILER=$(HOST)-gcc -DCMAKE_CXX_COMPILER=$(HOST)-g++ \
    -DCMAKE_INSTALL_PREFIX=$(host_prefix) -DTESTING=OFF -DSHARED=OFF ..
  endef
endif

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
