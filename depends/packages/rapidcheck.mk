package=rapidcheck
$(package)_version=d9482c683429fe79122e3dcab14c9655874aeb8e
$(package)_download_path=https://github.com/emil-e/rapidcheck/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=b9ee8955b175fd3c0757ebd887bb075541761af08b0c28391b7c6c0685351f6b

define $(package)_config_cmds
  cmake -DCMAKE_INSTALL_PREFIX=$($(package)_staging_dir)$(host_prefix) -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=true -DRC_ENABLE_BOOST_TEST=ON -B .
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
