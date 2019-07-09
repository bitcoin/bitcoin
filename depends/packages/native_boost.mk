package=native_boost
$(package)_version=$(boost_version)
$(package)_download_path=$(boost_download_path)
$(package)_file_name=$(boost_file_name)
$(package)_sha256_hash=$(boost_sha256_hash)

define $(package)_config_cmds
  ./bootstrap.sh --prefix=$($($(package)_type)_prefix) --with-libraries=headers
endef

define $(package)_stage_cmds
  ./b2 -d0 --prefix=$($(package)_staging_prefix_dir) install
endef
