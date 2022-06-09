package=capnp
$(package)_version=$(native_$(package)_version)
$(package)_download_path=$(native_$(package)_download_path)
$(package)_download_file=$(native_$(package)_download_file)
$(package)_file_name=$(native_$(package)_file_name)
$(package)_sha256_hash=$(native_$(package)_sha256_hash)
$(package)_dependencies=native_$(package)

define $(package)_set_vars :=
$(package)_config_opts := --with-external-capnp
$(package)_config_opts += CAPNP="$$(native_capnp_prefixbin)/capnp"
$(package)_config_opts += CAPNP_CXX="$$(native_capnp_prefixbin)/capnp-c++"
$(package)_config_opts_android := --disable-shared
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
