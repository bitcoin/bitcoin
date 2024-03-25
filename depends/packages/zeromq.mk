package=zeromq
$(package)_version=4.3.5
$(package)_download_path=https://github.com/zeromq/libzmq/releases/download/v$($(package)_version)/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=6653ef5910f17954861fe72332e68b03ca6e4d9c7160eb3a8de5a5a913bfab43
$(package)_patches=remove_libstd_link.patch fix_cmake_build.patch

define $(package)_set_vars
  $(package)_config_opts = --without-docs --disable-shared --disable-valgrind
  $(package)_config_opts += --disable-perf --disable-curve-keygen --disable-curve --disable-libbsd
  $(package)_config_opts += --without-libsodium --without-libgssapi_krb5 --without-pgm --without-norm --without-vmci
  $(package)_config_opts += --disable-libunwind --disable-radix-tree --without-gcov --disable-dependency-tracking
  $(package)_config_opts += --disable-Werror --disable-drafts --enable-option-checking
endef

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/remove_libstd_link.patch && \
  patch -p1 < $($(package)_patch_dir)/fix_cmake_build.patch
endef

define $(package)_config_cmds
  ./autogen.sh && \
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub config && \
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE) src/libzmq.la
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install-libLTLIBRARIES install-includeHEADERS install-pkgconfigDATA
endef

define $(package)_postprocess_cmds
  rm -rf bin share lib/*.la
endef
