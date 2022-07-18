package := double_conversion
$(package)_version := 3.2.0
$(package)_download_path := https://github.com/google/double-conversion/archive/refs/tags
$(package)_file_name := v$($(package)_version).tar.gz
$(package)_sha256_hash := 3dbcdf186ad092a8b71228a5962009b5c96abde9a315257a3452eb988414ea3b

define $(package)_config_cmds
  $($(package)_cmake) . -DBUILD_SHARED_LIBS=ON
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm -rf lib/cmake/
endef
