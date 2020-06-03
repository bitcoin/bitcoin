package=veriblock-pop-cpp
$(package)_version=37f98a8a4530953fafbfd601d6aa07907d35aee3
$(package)_download_path=https://github.com/VeriBlock/alt-integration-cpp/archive/
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=8a32162cffbea559dd5cc753f62f2324dfd2f92de56336929d5ad19b8868fbb6
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
