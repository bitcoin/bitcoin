package=qtsowrap
$(package)_version=0.0.1
$(package)_download_path=https://github.com/laanwj/qtsowrap/archive/
$(package)_file_name=28ceb2d75c036b0309f4c1e73aa8f4e50d704f65.tar.gz
#$(package)_file_name=$(package)-$($(package)_version).tar.xz
$(package)_sha256_hash=cdb9f803be1bc704674b9c900158914ab702e027b68e38bc1eb471bc284b11d4
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
