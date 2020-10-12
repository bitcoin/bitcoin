package=curl
$(package)_version=7.69.1
$(package)_dependencies=gnutls zlib
$(package)_download_path=https://curl.haxx.se/download
$(package)_file_name=curl-$($(package)_version).tar.gz
$(package)_sha256_hash=01ae0c123dee45b01bbaef94c0bc00ed2aec89cb2ee0fd598e0d302a6b5e0a98

# default settings
$(package)_config_opts=LIBS="-lnettle -lhogweed -lgnutls -lz" --without-nghttp2 --disable-shared --enable-static --disable-ftp --without-ssl --with-gnutls="$(PREFIX_DIR)" --with-zlib="$(PREFIX_DIR)" --disable-ntlm-wb --disable-file --disable-ldap --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --enable-proxy --without-ca-path --without-ca-bundle --with-ca-fallback --enable-threaded-resolver
$(package)_config_opts_linux=--with-pic

# mingw specific settings
$(package)_config_opts_mingw32=LIBS="-lcrypt32 -lnettle -lhogweed -lgnutls -lz" --without-nghttp2 CFLAGS="-DCURL_STATICLIB" --disable-shared --enable-static --disable-ftp --without-ssl --with-gnutls="$(PREFIX_DIR)" --with-zlib="$(PREFIX_DIR)" --disable-ntlm-wb --disable-file --disable-ldap --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --enable-proxy --without-ca-path --without-ca-bundle --with-ca-fallback

# darwin specific settings
$(package)_config_opts_darwin=LIBS="-lcrypt32 -lnettle -lhogweed -lgnutls -lz" --with-sysroot="$(DARWIN_SDK_PATH)" --disable-shared --enable-static --disable-ftp --without-gnutls --with-zlib="$(PREFIX_DIR)" --with-darwinssl --without-ssl --disable-ntlm-wb --disable-file --disable-ldap --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --enable-proxy --enable-threaded-resolver

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