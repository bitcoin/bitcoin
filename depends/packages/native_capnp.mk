package=native_capnp
$(package)_version=1.0.1
$(package)_download_path=https://capnproto.org/
$(package)_download_file=capnproto-c++-$($(package)_version).tar.gz
$(package)_file_name=capnproto-cxx-$($(package)_version).tar.gz
$(package)_sha256_hash=0f7f4b8a76a2cdb284fddef20de8306450df6dd031a47a15ac95bc43c3358e09

define $(package)_set_vars
  $(package)_config_opts = --without-openssl
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
