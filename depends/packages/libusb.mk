package=libusb
$(package)_version=1.0.21
$(package)_download_path=https://github.com/libusb/libusb/archive/
$(package)_file_name=v$($(package)_version).tar.gz
$(package)_sha256_hash=1a5b08c05bc5e38c81c2d59c29954d5916646f4ff46f51381b3f624384e4ac01

define $(package)_set_vars
  $(package)_config_opts=--disable-shared --enable-udev
  $(package)_config_opts_linux=--with-pic
endef

define $(package)_preprocess_cmds
  cd $($(package)_build_subdir); ./bootstrap.sh
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
