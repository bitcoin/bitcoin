package=gmp
$(package)_version=6.0.0a
$(package)_download_path=https://gmplib.org/download/gmp
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=7f8e9a804b9c6d07164cf754207be838ece1219425d64e28cfa3e70d5c759aaf
$(package)_patches=arm_gmp_build_fix.patch darwin_gmp_build_fix.patch

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/arm_gmp_build_fix.patch && \
  patch -p1 < $($(package)_patch_dir)/darwin_gmp_build_fix.patch
endef

define $(package)_set_vars
  $(package)_config_opts=--disable-shared CC_FOR_BUILD=$(build_CC)
  $(package)_config_opts_x86_64_darwin=--with-pic
  $(package)_config_opts_x86_64_linux=--with-pic
  $(package)_config_opts_arm_linux=--with-pic
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
