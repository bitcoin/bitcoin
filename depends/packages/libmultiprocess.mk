package=libmultiprocess
$(package)_version=$(native_$(package)_version)
$(package)_download_path=$(native_$(package)_download_path)
$(package)_file_name=$(native_$(package)_file_name)
$(package)_sha256_hash=$(native_$(package)_sha256_hash)
$(package)_dependencies=native_$(package) capnp
ifneq ($(host),$(build))
$(package)_dependencies += native_capnp
endif

define $(package)_set_vars :=
ifneq ($(host),$(build))
$(package)_config_opts := -DCAPNP_EXECUTABLE="$$(native_capnp_prefixbin)/capnp"
$(package)_config_opts += -DCAPNPC_CXX_EXECUTABLE="$$(native_capnp_prefixbin)/capnpc-c++"
endif
$(package)_cxxflags += -ffile-prefix-map=$$($(package)_extract_dir)=/usr
endef

define $(package)_config_cmds
  $($(package)_cmake) .
endef

define $(package)_build_cmds
  $(MAKE) multiprocess
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install-lib
endef
