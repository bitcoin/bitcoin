package=native_libmultiprocess
$(package)_version=v5.0
$(package)_download_path=https://github.com/bitcoin-core/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=401984715b271a3446e1910f21adf048ba390d31cc93cc3073742e70d56fa3ea
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
