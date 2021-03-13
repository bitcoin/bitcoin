package=libcurl
$(package)_version=7.75.0
$(package)_dependencies=gnutls zlib
$(package)_download_path=https://curl.haxx.se/download
$(package)_file_name=curl-$($(package)_version).tar.gz
$(package)_sha256_hash=4d51346fe621624c3e4b9f86a8fd6f122a143820e17889f59c18f245d2d8e7a6

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

define $(package)_postprocess_cmds
  rm lib/*.la
endef