package=zeromq
$(package)_version=4.0.4
$(package)_download_path=http://download.zeromq.org
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=1ef71d46e94f33e27dd5a1661ed626cd39be4d2d6967792a275040e34457d399

define $(package)_set_vars
  $(package)_config_opts=--without-documentation --disable-shared
  $(package)_config_opts_linux=--with-pic
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE) -C src
endef

define $(package)_stage_cmds
  $(MAKE) -C src DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm -rf bin share
endef
