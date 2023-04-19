package=libnatpmp
$(package)_version=07004b97cf691774efebe70404cf22201e4d330d
$(package)_download_path=https://github.com/miniupnp/libnatpmp/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=9321953ceb39d07c25463e266e50d0ae7b64676bb3a986d932b18881ed94f1fb
$(package)_build_subdir=build

define $(package)_set_vars
  $(package)_config_opts += -DCMAKE_POSITION_INDEPENDENT_CODE=ON
endef

define $(package)_config_cmds
  $($(package)_cmake) -S .. -B .
endef

define $(package)_build_cmds
  $(MAKE) natpmp
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/include $($(package)_staging_prefix_dir)/lib &&\
  install ../*.h $($(package)_staging_prefix_dir)/include &&\
  install libnatpmp.a $($(package)_staging_prefix_dir)/lib
endef
