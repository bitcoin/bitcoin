package=zeromq
$(package)_version=4.3.4
$(package)_download_path=https://github.com/zeromq/libzmq/releases/download/v$($(package)_version)/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=c593001a89f5a85dd2ddf564805deb860e02471171b3f204944857336295c3e5
$(package)_patches=remove_libstd_link.patch netbsd_kevent_void.patch

define $(package)_set_vars
  $(package)_config_opts = --without-docs --disable-shared --disable-valgrind
  $(package)_config_opts += --disable-perf --disable-curve-keygen --disable-curve --disable-libbsd
  $(package)_config_opts += --without-libsodium --without-libgssapi_krb5 --without-pgm --without-norm --without-vmci
  $(package)_config_opts += --disable-libunwind --disable-radix-tree --without-gcov --disable-dependency-tracking
  $(package)_config_opts += --disable-Werror --disable-drafts --enable-option-checking
  $(package)_config_opts_linux=--with-pic
  $(package)_config_opts_freebsd=--with-pic
  $(package)_config_opts_netbsd=--with-pic
  $(package)_config_opts_openbsd=--with-pic
  $(package)_config_opts_android=--with-pic
endef

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/remove_libstd_link.patch && \
  patch -p1 < $($(package)_patch_dir)/netbsd_kevent_void.patch
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
