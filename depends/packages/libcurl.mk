package=curl
$(package)_version=7.72.0
$(package)_dependencies=gnutls zlib
$(package)_download_path=https://curl.haxx.se/download
$(package)_file_name=curl-$($(package)_version).tar.gz
$(package)_sha256_hash=d4d5899a3868fbb6ae1856c3e55a32ce35913de3956d1973caccd37bd0174fa2

# default settings
$(package)_config_opts=LIBS="-lnettle -lhogweed -lgnutls" --without-nghttp2 --disable-shared --enable-static --disable-ftp --without-ssl --with-gnutls --disable-ntlm-wb --disable-file --disable-ldap --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --enable-proxy --without-ca-path --without-ca-bundle --with-ca-fallback --enable-threaded-resolver --with-zlib

# mingw specific settings
$(package)_config_opts_mingw32=LIBS="-lcrypt32 -lnettle -lhogweed -lgnutls" --without-nghttp2 CFLAGS="-DCURL_STATICLIB" --disable-shared --enable-static --disable-ftp --without-ssl --with-gnutls --disable-ntlm-wb --disable-file --disable-ldap --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --enable-proxy --without-ca-path --without-ca-bundle --with-ca-fallback --with-zlib

# darwin specific settings
$(package)_config_opts_darwin=--with-sysroot="$(DARWIN_SDK_PATH)" --disable-shared --enable-static --disable-ftp --without-gnutls --with-darwinssl --without-ssl --disable-ntlm-wb --disable-file --disable-ldap --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --enable-proxy --enable-threaded-resolver --with-zlib

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