package=hidapi
$(package)_version=0.8.0-rc1
$(package)_download_path=https://github.com/signal11/hidapi/archive/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=3c147200bf48a04c1e927cd81589c5ddceff61e6dac137a605f6ac9793f4af61
$(package)_linux_dependencies=libusb

define $(package)_set_vars
  $(package)_config_opts=--disable-shared
  $(package)_config_opts_linux=--with-pic
endef

define $(package)_preprocess_cmds
  sed -i '64,67d' configure.ac; \
  sed -i '64iLIBS_HIDRAW_PR+=" -ludev"' configure.ac; \
  cd $($(package)_build_subdir); ./bootstrap
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
