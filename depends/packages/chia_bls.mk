package=chia_bls
$(package)_version=v20201015
# It's actually from https://github.com/Chia-Network/bls-signatures, but we have so many patches atm that it's forked
$(package)_download_path=https://github.com/syscoin/bls-signatures/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=c6698a1e8aa2ac7842d5ad902267b2ba65a4cfa8996372e7e5c0c83758e669dd
$(package)_dependencies=gmp
#$(package)_patches=...TODO (when we switch back to https://github.com/Chia-Network/bls-signatures)

#define $(package)_preprocess_cmds
#  for i in $($(package)_patches); do patch -N -p1 < $($(package)_patch_dir)/$$$$i; done
#endef

define $(package)_set_vars
  $(package)_cflags=-DSTLIB=ON -DSHLIB=OFF -DSTBIN=ON
  $(package)_cflags_linux=-DOPSYS=LINUX -DCMAKE_SYSTEM_NAME=Linux
  $(package)_cflags_darwin=-DOPSYS=MACOSX -DCMAKE_SYSTEM_NAME=Darwin
  $(package)_cflags_mingw32=-DOPSYS=WINDOWS -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_SHARED_LIBRARY_LINK_C_FLAGS="" -DCMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS=""
  $(package)_cflags_i686+=-DWSIZE=32
  $(package)_cflags_x86_64+=-DWSIZE=64
  $(package)_cflags_arm+=-DWSIZE=32
  $(package)_cflags_armv7l+=-DWSIZE=32
endef

define $(package)_config_cmds
  mkdir -p build && cd build && \
  $($(package)_cmake) ../ $($(package)_cflags)
endef

define $(package)_build_cmds
  cd build && $(MAKE) $($(package)_build_opts)
endef

define $(package)_stage_cmds
  cd build && $(MAKE) install
endef