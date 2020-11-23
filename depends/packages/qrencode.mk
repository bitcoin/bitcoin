package=qrencode
$(package)_version=3.4.4
$(package)_download_path=https://fukuchi.org/works/qrencode/
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=efe5188b1ddbcbf98763b819b146be6a90481aac30cfc8d858ab78a19cde1fa5

define $(package)_set_vars
$(package)_config_opts=--disable-shared --without-tools --without-tests --disable-sdltest
$(package)_config_opts += --disable-gprof --disable-gcov --disable-mudflap
$(package)_config_opts += --disable-dependency-tracking --enable-option-checking
$(package)_config_opts_linux=--with-pic
$(package)_config_opts_android=--with-pic
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
