package=native_libmultiprocess
$(package)_version=1af83d15239ccfa7e47b8764029320953dd7fdf1
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=e5587d3feedc7f8473f178a89b94163a11076629825d664964799bbbd5844da5
$(package)_dependencies=native_capnp

define $(package)_config_cmds
  $($(package)_cmake) .
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install-bin
endef
