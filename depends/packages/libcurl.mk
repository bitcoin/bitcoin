package=libcurl
$(package)_version=7.73.0
$(package)_dependencies=gnutls zlib
$(package)_download_path=https://curl.haxx.se/download
$(package)_file_name=curl-$($(package)_version).tar.gz
$(package)_sha256_hash=ba98332752257b47b9dea6d8c0ad25ec1745c20424f1dd3ff2c99ab59e97cf91

# default settings
$(package)_cflags_darwin="-mmacosx-version-min=$(OSX_MIN_VERSION)"
$(package)_config_opts=--with-zlib="$(host_prefix)" --disable-shared --enable-static --disable-ftp --disable-file --disable-ldap --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --enable-proxy --without-ssl --with-gnutls="$(host_prefix)" --without-ca-path --without-ca-bundle --with-ca-fallback --disable-telnet
$(package)_config_opts_linux=LIBS="-lnettle -lhogweed -lgmp" --without-ssl --enable-threaded-resolver

# mingw specific settings
$(package)_config_opts_mingw32=LIBS="-lcrypt32 -lnettle -lhogweed -lgmp" --without-ssl

# darwin specific settings
$(package)_config_opts_darwin=LIBS="-lnettle -lhogweed -lgmp" --with-sysroot="$(DARWIN_SDK_PATH)" --without-ssl --enable-threaded-resolver

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_preprocess_cmds
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub .
endef

define $(package)_postprocess_cmds
  rm lib/*.la
endef