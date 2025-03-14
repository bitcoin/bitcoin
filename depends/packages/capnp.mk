package=capnp
$(package)_version=$(native_$(package)_version)
$(package)_download_path=$(native_$(package)_download_path)
$(package)_download_file=$(native_$(package)_download_file)
$(package)_file_name=$(native_$(package)_file_name)
$(package)_sha256_hash=$(native_$(package)_sha256_hash)
$(package)_patches=abi_placement_new.patch

define $(package)_set_vars
  $(package)_config_opts := -DBUILD_TESTING=OFF
  $(package)_config_opts += -DWITH_OPENSSL=OFF
  $(package)_config_opts += -DWITH_ZLIB=OFF
  $(package)_cxxflags += -fdebug-prefix-map=$($(package)_extract_dir)=/usr -fmacro-prefix-map=$($(package)_extract_dir)=/usr
endef

define $(package)_preprocess_cmds
  patch -p2 < $($(package)_patch_dir)/abi_placement_new.patch
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
