package=capnp
$(package)_version=$(native_$(package)_version)
$(package)_download_path=$(native_$(package)_download_path)
$(package)_download_file=$(native_$(package)_download_file)
$(package)_file_name=$(native_$(package)_file_name)
$(package)_sha256_hash=$(native_$(package)_sha256_hash)

# Hardcode library install path to "lib" to match the PKG_CONFIG_PATH
# setting in depends/config.site.in, which also hardcodes "lib".
# Without this setting, cmake by default would use the OS library
# directory, which might be "lib64" or something else, not "lib", on multiarch systems.
define $(package)_set_vars :=
  $(package)_config_opts := -DBUILD_TESTING=OFF
  $(package)_config_opts += -DWITH_OPENSSL=OFF
  $(package)_config_opts += -DWITH_ZLIB=OFF
  $(package)_config_opts += -DCMAKE_INSTALL_LIBDIR=lib/
endef

define $(package)_config_cmds
  $($(package)_cmake) .
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm -rf lib/pkgconfig
endef
