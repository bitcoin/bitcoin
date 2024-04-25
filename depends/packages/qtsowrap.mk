package=qtsowrap
$(package)_version=0.0.1
$(package)_download_path=https://github.com/laanwj/qtsowrap/archive/
$(package)_file_name=8825b14e8db51a1ae707c58448ec36dfa7ab7be2.tar.gz
$(package)_sha256_hash=26f99e32d0b9f84f588d5fd6e129812418759d0fd551f6a35367d61974ea424a
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
