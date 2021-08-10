package=immer
$(package)_version=v0.6.2
$(package)_download_path=https://github.com/arximboldi/immer/archive
$(package)_download_file=$($(package)_version).tar.gz
$(package)_file_name=$(package)-$($(package)_download_file)
$(package)_sha256_hash=c3bb8847034437dee64adacb04e1e0163ae640b596c582eb4c0aa1d7c6447cd7
$(package)_dependencies=cmake boost

define $(package)_fetch_cmds
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_download_file),$($(package)_file_name),$($(package)_sha256_hash))
endef

define $(package)_set_vars
  $(package)_config_opts=-DCMAKE_INSTALL_INCLUDEDIR=$($(package)_staging_dir)/$(host_prefix)/include
  $(package)_config_opts+=-DCMAKE_INSTALL_LIBDIR=$($(package)_staging_dir)/$(host_prefix)/lib
  $(package)_config_opts_linux=-DCMAKE_SYSTEM_NAME=Linux
  $(package)_config_opts_darwin=-DCMAKE_SYSTEM_NAME=Darwin
  $(package)_config_opts_mingw32=-DCMAKE_SYSTEM_NAME=Windows -DCMAKE_SHARED_LIBRARY_LINK_C_FLAGS=""
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
  mkdir -p "$($(package)_dependencies)" && cd "$($(package)_dependencies)" && \
  $(host_prefix)/bin/cmake ../ $($(package)_config_opts)
endef

define $(package)_build_cmds
  cd "$($(package)_dependencies)" && \
  $(MAKE) $($(package)_build_opts)
endef

define $(package)_stage_cmds
  cd "$($(package)_dependencies)" && \
  $(MAKE) install
endef
