package=gmp
$(package)_version=6.3.0
$(package)_download_path=https://ftp.gnu.org/gnu/gmp
$(package)_file_name=gmp-$($(package)_version).tar.bz2
$(package)_sha256_hash=ac28211a7cfb609bae2e2c8d6058d66c8fe96434f740cf6fe2e47b000d1c20cb
$(package)_patches=include_ldflags_in_configure.patch

define $(package)_set_vars
$(package)_config_opts += --disable-shared --enable-cxx --enable-fat
$(package)_cflags_aarch64 += -march=armv8-a
$(package)_cflags_armv7l += -march=armv7-a
$(package)_cflags_x86_64 += -march=x86-64
endef

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/include_ldflags_in_configure.patch
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
