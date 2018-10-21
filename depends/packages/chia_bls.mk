package=chia_bls
$(package)_version=ef06290c1d92eea79b46af135bcc823b019612a6
# It's actually from https://github.com/Chia-Network/bls-signatures, but we have so many patches atm that it's forked
$(package)_download_path=https://github.com/codablock/bls-signatures/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=1e665359fdb946efce943340a324534f72832ecde97c4ebd4ad189f63e029300
$(package)_dependencies=gmp
#$(package)_patches=...TODO (when we switch back to https://github.com/Chia-Network/bls-signatures)

#define $(package)_preprocess_cmds
#  for i in $($(package)_patches); do patch -N -p1 < $($(package)_patch_dir)/$$$$i; done
#endef

define $(package)_set_vars
  $(package)_config_opts=-DCMAKE_INSTALL_PREFIX=$($(package)_staging_dir)/$(host_prefix)
  $(package)_config_opts+= -DCMAKE_PREFIX_PATH=$(host_prefix)
  $(package)_config_opts+= -DSTLIB=ON -DSHLIB=OFF -DSTBIN=ON
  $(package)_config_opts_linux=-DOPSYS=LINUX -DCMAKE_SYSTEM_NAME=Linux
  $(package)_config_opts_darwin=-DOPSYS=MACOSX -DCMAKE_SYSTEM_NAME=Darwin
  $(package)_config_opts_mingw32=-DOPSYS=WINDOWS -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_SHARED_LIBRARY_LINK_C_FLAGS="" -DCMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS=""
  $(package)_config_opts_i686+= -DWSIZE=32
  $(package)_config_opts_x86_64+= -DWSIZE=64
  $(package)_config_opts_arm+= -DWSIZE=32
  $(package)_config_opts_debug=-DDEBUG=ON -DCMAKE_BUILD_TYPE=Debug

  ifneq ($(darwin_native_toolchain),)
    $(package)_config_opts_darwin+= -DCMAKE_AR="$(host_prefix)/native/bin/$($(package)_ar)"
    $(package)_config_opts_darwin+= -DCMAKE_RANLIB="$(host_prefix)/native/bin/$($(package)_ranlib)"
  endif
endef

define $(package)_config_cmds
  export CC="$($(package)_cc)" && \
  export CXX="$($(package)_cxx)" && \
  export CFLAGS="$($(package)_cflags) $($(package)_cppflags)" && \
  export CXXFLAGS="$($(package)_cxxflags) $($(package)_cppflags)" && \
  export LDFLAGS="$($(package)_ldflags)" && \
  mkdir -p build && cd build && \
  cmake ../ $($(package)_config_opts)
endef

define $(package)_build_cmds
  cd build && \
  $(MAKE) $($(package)_build_opts)
endef

define $(package)_stage_cmds
  cd build && \
  $(MAKE) install
endef
