package=native_libmultiprocess
$(package)_version=6aca5f389bacf2942394b8738bbe15d6c9edfb9b
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=2efeed53542bc1d8af3291f2b6f0e5d430d86a5e04e415ce33c136f2c226a51d
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
