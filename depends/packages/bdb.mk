package=bdb
$(package)_version=4.8.30
$(package)_download_path=https://download.oracle.com/berkeley-db
$(package)_file_name=db-$($(package)_version).NC.tar.gz
$(package)_sha256_hash=12edc0df75bf9abd7f82f821795bcee50f42cb2e5f76a6a281b85732798364ef
$(package)_build_subdir=build_unix
$(package)_patches=clang_cxx_11.patch

define $(package)_set_vars
$(package)_config_opts=--disable-shared --enable-cxx --disable-replication --enable-option-checking
$(package)_config_opts_mingw32=--enable-mingw
$(package)_cflags+=-Wno-error=implicit-function-declaration -Wno-error=format-security -Wno-error=implicit-int
$(package)_cflags_netbsd+=-Wno-error=int-conversion
$(package)_cppflags_freebsd=-D_XOPEN_SOURCE=600 -D__BSD_VISIBLE=1
$(package)_cppflags_netbsd=-D_XOPEN_SOURCE=600
$(package)_cppflags_mingw32=-DUNICODE -D_UNICODE
endef

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/clang_cxx_11.patch && \
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub dist
endef

define $(package)_config_cmds
  ../dist/$($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE) libdb_cxx-4.8.a libdb-4.8.a
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install_lib install_include
endef
