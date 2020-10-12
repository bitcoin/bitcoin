package=curl
$(package)_version=7.69.1
$(package)_dependencies=gnutls zlib
$(package)_download_path=https://curl.haxx.se/download
$(package)_file_name=curl-$($(package)_version).tar.gz
$(package)_sha256_hash=ca2feeb8ef13368ce5d5e5849a5fd5e2dd4755fecf7d8f0cc94000a4206fb8e7

# default settings
$(package)_config_opts=--without-nghttp2 --disable-shared --enable-static --disable-ftp --without-ssl --with-gnutls="$(PREFIX_DIR)" --with-zlib="$(PREFIX_DIR)" --disable-ntlm-wb --disable-file --disable-ldap --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --enable-proxy --without-ca-path --without-ca-bundle --with-ca-fallback --enable-threaded-resolver
$(package)_config_opts_linux=--with-pic

# mingw specific settings
$(package)_config_opts_mingw32=--without-nghttp2 CFLAGS="-DCURL_STATICLIB" --disable-shared --enable-static --disable-ftp --without-ssl --with-gnutls="$(PREFIX_DIR)" --with-zlib="$(PREFIX_DIR)" --disable-ntlm-wb --disable-file --disable-ldap --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --enable-proxy --without-ca-path --without-ca-bundle --with-ca-fallback

# darwin specific settings
$(package)_config_opts_darwin=--with-sysroot="$(DARWIN_SDK_PATH)" --disable-shared --enable-static --disable-ftp --without-gnutls --with-zlib="$(PREFIX_DIR)" --with-darwinssl --without-ssl --disable-ntlm-wb --disable-file --disable-ldap --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --enable-proxy --enable-threaded-resolver

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