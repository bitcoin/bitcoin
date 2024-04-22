package=qtsowrap
$(package)_version=0.0.1
$(package)_download_path=https://github.com/laanwj/qtsowrap/archive/
$(package)_file_name=474a0d7b0aded621d10ebe6ffd0e69622ec5337c.tar.gz
#$(package)_file_name=$(package)-$($(package)_version).tar.xz
$(package)_sha256_hash=d0ebb0794fddf1213eca54e1b51dba1aae024bd557cc53c73963744f6791d0ce
$(package)_dependencies=
$(package)_patches =

define $(package)_set_vars :=
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
endef
