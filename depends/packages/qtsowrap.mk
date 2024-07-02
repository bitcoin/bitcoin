package=qtsowrap
$(package)_version=0.1.0
$(package)_download_path=https://github.com/laanwj/qtsowrap/releases/download/v$($(package)_version)/
$(package)_file_name=qtsowrap-$($(package)_version).tar.gz
$(package)_sha256_hash=a26738d10263fcca3d5e04d81a0902e9552f15c29174f1b3c22234b2615e60ff
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
