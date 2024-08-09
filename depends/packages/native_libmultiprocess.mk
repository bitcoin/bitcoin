package=native_libmultiprocess
$(package)_version=a9e16da55e880e5a0aed5008307ea56edc33fbd1
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=588afeaa8751ef56fe5bfdf1e40587809bcb617e4d2825064b185860977a4b5f
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
