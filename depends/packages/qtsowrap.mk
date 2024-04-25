package=qtsowrap
$(package)_version=0.0.1
$(package)_download_path=https://github.com/laanwj/qtsowrap/archive/
$(package)_file_name=996b672c3ad5736d962e04618fc86f75df92647e.tar.gz
$(package)_sha256_hash=e12b78a1988e7b8e28b14b7f2d2ad4f33a465b9e9775b4a713f9c82efb99fcaf
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
