package=p11kit
$(package)_version=0.23.21
$(package)_dependencies=nettle
$(package)_download_path=https://github.com/p11-glue/p11-kit/archive/
$(package)_file_name=$(package)-$($(package)_version).tar.xz
$(package)_sha256_hash=0361bcc55858618625a8e99e7fe9069f81514849b7b51ade51f8117d3ad31d88


# default settings
$(package)_config_opts=--disable-shared --enable-static --without-libtasn1

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm lib/*.la
endef