package=expat
$(package)_version=2.4.8
$(package)_download_path=https://github.com/libexpat/libexpat/releases/download/R_$(subst .,_,$($(package)_version))/
$(package)_file_name=$(package)-$($(package)_version).tar.xz
$(package)_sha256_hash=f79b8f904b749e3e0d20afeadecf8249c55b2e32d4ebb089ae378df479dcaf25

# -D_DEFAULT_SOURCE defines __USE_MISC, which exposes additional
# definitions in endian.h, which are required for a working
# endianess check in configure when building with -flto.
define $(package)_set_vars
  $(package)_config_opts=--disable-shared --without-docbook --without-tests --without-examples
  $(package)_config_opts += --disable-dependency-tracking --enable-option-checking
  $(package)_config_opts += --without-xmlwf
  $(package)_config_opts_linux=--with-pic
  $(package)_cppflags += -D_DEFAULT_SOURCE
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
  rm -rf share lib/cmake lib/*.la
endef
