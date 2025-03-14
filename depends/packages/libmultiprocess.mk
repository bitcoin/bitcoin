package=libmultiprocess
$(package)_version=$(native_$(package)_version)
$(package)_download_path=$(native_$(package)_download_path)
$(package)_file_name=$(native_$(package)_file_name)
$(package)_sha256_hash=$(native_$(package)_sha256_hash)
$(package)_dependencies=native_$(package) capnp
ifneq ($(host),$(build))
$(package)_dependencies += native_capnp
endif

# Hardcode library install path to "lib" to match the PKG_CONFIG_PATH
# setting in depends/config.site.in, which also hardcodes "lib".
# Without this setting, cmake by default would use the OS library
# directory, which might be "lib64" or something else, not "lib", on multiarch systems.
define $(package)_set_vars
$(package)_config_opts += -DCMAKE_INSTALL_LIBDIR=lib/
$(package)_config_opts += -DCMAKE_POSITION_INDEPENDENT_CODE=ON
ifneq ($(host),$(build))
$(package)_config_opts := -DCAPNP_EXECUTABLE="$$(native_capnp_prefixbin)/capnp"
$(package)_config_opts += -DCAPNPC_CXX_EXECUTABLE="$$(native_capnp_prefixbin)/capnpc-c++"
endif
endef

define $(package)_config_cmds
  $($(package)_cmake) .
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install-lib
endef
