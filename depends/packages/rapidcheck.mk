package=rapidcheck
$(package)_version=10fc0cb
$(package)_download_path=https://github.com/MarcoFalke/rapidcheck/archive
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=9640926223c00af45bce4c7df8b756b5458a89b2ba74cfe3e404467f13ce26df

define $(package)_config_cmds
  cmake -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=true .
endef

define $(package)_build_cmds
  $(MAKE) && \
  mkdir -p $($(package)_staging_dir)$(host_prefix)/include && \
  cp -a include/* $($(package)_staging_dir)$(host_prefix)/include/ && \
  cp -a extras/boost_test/include/rapidcheck/* $($(package)_staging_dir)$(host_prefix)/include/rapidcheck/ && \
  mkdir -p $($(package)_staging_dir)$(host_prefix)/lib && \
  cp -a librapidcheck.a $($(package)_staging_dir)$(host_prefix)/lib/
endef
