package=native_protobuf
$(package)_version=3.17.3
$(package)_download_path=https://github.com/google/protobuf/releases/download/v$($(package)_version)
$(package)_file_name=protobuf-cpp-$($(package)_version).tar.gz
$(package)_sha256_hash=51cec99f108b83422b7af1170afd7aeb2dd77d2bcbb7b6bad1f92509e9ccf8cb

define $(package)_set_vars
$(package)_config_opts=--disable-shared --without-zlib
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE) -C src protoc
endef

define $(package)_stage_cmds
  $(MAKE) -C src DESTDIR=$($(package)_staging_dir) install-strip
endef

define $(package)_postprocess_cmds
  rm -rf lib include
endef
