package=qrencode
$(package)_version=4.1.1
$(package)_download_path=https://fukuchi.org/works/qrencode/
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=e455d9732f8041cf5b9c388e345a641fd15707860f928e94507b1961256a6923

define $(package)_set_vars
$(package)_config_opts=--disable-shared --without-tools --without-tests --without-png
$(package)_config_opts += --disable-gprof --disable-gcov --disable-mudflap
$(package)_config_opts += --disable-dependency-tracking --enable-option-checking
$(package)_config_opts_linux=--with-pic
$(package)_config_opts_android=--with-pic
$(package)_cflags += -Wno-int-conversion -Wno-implicit-function-declaration
endef

define $(package)_preprocess_cmds
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub use
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

define $(package)_postprocess_cmds
  rm lib/*.la
endef
