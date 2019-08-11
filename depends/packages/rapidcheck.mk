package=rapidcheck
$(package)_version=3eb9b4ff69f4ff2d9932e8f852c2b2a61d7c20d3
$(package)_download_path=https://github.com/emil-e/rapidcheck/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=5fbf82755c9a647127e62563be079448ff8b1db9ca80a52a673dd9a88fdb714b

define $(package)_config_cmds
  cmake -DCMAKE_INSTALL_PREFIX=$($(package)_staging_dir)$(host_prefix) -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=true -DRC_INSTALL_ALL_EXTRAS=ON
endef

define $(package)_preprocess_cmds
  sed -i.old 's/ -Wall//' CMakeLists.txt
endef

define $(package)_build_cmds
  $(MAKE) rapidcheck
endef

define $(package)_stage_cmds
  $(MAKE) rapidcheck install
endef
