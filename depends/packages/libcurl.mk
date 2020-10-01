package=curl
$(package)_version=7.67.0
$(package)_dependencies=openssl
$(package)_download_path=https://curl.haxx.se/download/
$(package)_file_name=curl-$($(package)_version).tar.gz
$(package)_sha256_hash=52af3361cf806330b88b4fe6f483b6844209d47ae196ac46da4de59bb361ab02

# default settings
$(package)_config_env_default=LD_LIBRARY_PATH="$(PREFIX_DIR)lib" PKG_CONFIG_LIBDIR="$(PREFIX_DIR)lib/pkgconfig" CPPFLAGS="-I$(PREFIX_DIR)include" LDFLAGS="-L$(PREFIX_DIR)lib"
$(package)_config_opts_default=--disable-maintainer-mode --disable-ftp --disable-file --disable-ldap --disable-ldaps --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-gopher --enable-proxy --without-ca-path --without-ca-bundle --disable-manual --without-polarssl --without-mbedtls --without-cyassl --without-nss --without-axtls --without-libssh2 --without-librtmp --without-nghttp2 --without-ssl --with-gnutls --disable-verbose --disable-smtp --disable-libcurl-option --enable-threaded-resolver

# mingw specific settings
$(package)_config_env_mingw32=LD_LIBRARY_PATH="$(PREFIX_DIR)lib" PKG_CONFIG_LIBDIR="$(PREFIX_DIR)lib/pkgconfig" CPPFLAGS="-I$(PREFIX_DIR)include -DCURL_STATIC_LIB -static" LDFLAGS="-L$(PREFIX_DIR)lib"
$(package)_config_opts_mingw32=--disable-ftp --without-ssl --with-gnutls --disable-file --disable-ldap --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --enable-proxy --without-ca-path --without-ca-bundle --with-ca-fallback --disable-telnet
$(package)_config_env_x86_64-w64-mingw32=$($(package)_config_env_mingw32)
$(package)_config_env_i686-w64-mingw32=$($(package)_config_env_mingw32)
$(package)_config_opts_x86_64-w64-mingw32=$($(package)_config_opts_mingw32)
$(package)_config_opts_i686-w64-mingw32=$($(package)_config_opts_mingw32)

#mingw dll specific settings
ifeq ($(BUILD_DLL), 1)
$(package)_config_env_x86_64-w64-mingw32=LD_LIBRARY_PATH="$(PREFIX_DIR)lib" PKG_CONFIG_LIBDIR="$(PREFIX_DIR)lib/pkgconfig" CPPFLAGS="-I$(PREFIX_DIR)include" LDFLAGS="-L$(PREFIX_DIR)lib"
endif

# 32-bit linux specific settings
$(package)_config_env_i686-pc-linux-gnu=LD_LIBRARY_PATH="$(PREFIX_DIR)lib" PKG_CONFIG_LIBDIR="$(PREFIX_DIR)lib/pkgconfig" CPPFLAGS="-I$(PREFIX_DIR)include -m32" LDFLAGS="-L$(PREFIX_DIR)lib -m32"
$(package)_config_opts_i686-pc-linux-gnu=--disable-maintainer-mode --disable-ftp --disable-file --disable-ldap --disable-ldaps --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-gopher --enable-proxy --without-ca-path --without-ca-bundle --disable-manual --without-polarssl --without-mbedtls --without-cyassl --without-nss --without-axtls --without-libssh2 --without-librtmp --without-nghttp2 --without-ssl --with-gnutls --disable-verbose --disable-smtp --disable-libcurl-option --enable-threaded-resolver

# 32 bit ARM gnueabihf settings
$(package)_config_opts_arm=--disable-maintainer-mode --disable-ftp --disable-file --disable-ldap --disable-ldaps --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-gopher --enable-proxy --with-ca-bundle=/etc/ssl/certs/ca-certificates.crt --with-ca-fallback --disable-manual --without-polarssl --without-mbedtls --without-cyassl --without-nss --without-axtls --without-libssh2 --without-librtmp --without-nghttp2 --without-ssl --with-gnutls --disable-verbose --disable-smtp --disable-libcurl-option --enable-threaded-resolver
$(package)_config_opts_arm-linux-gnueabihf=$($(package)_config_opts_arm)
$(package)_config_opts_aarch64-linux-gnu=$($(package)_config_opts_arm)

# darwin specific settings
$(package)_config_opts_darwin=--with-sysroot="$(DARWIN_SDK_PATH)" --disable-maintainer-mode --disable-ftp --disable-file --disable-ldap --disable-ldaps --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-gopher --enable-proxy --without-ca-path --without-ca-bundle --disable-manual --without-polarssl --without-mbedtls --without-cyassl --without-nss --without-axtls --without-libssh2 --without-librtmp --without-nghttp2 --without-ssl --with-gnutls --disable-verbose --disable-smtp --disable-libcurl-option --enable-threaded-resolver
$(package)_config_opts_x86_64-apple-darwin11=$($(package)_config_opts_darwin)

# set settings based on host
$(package)_config_env = $(if $($(package)_config_env_$(HOST)), $($(package)_config_env_$(HOST)), $($(package)_config_env_default))
$(package)_config_opts = $(if $($(package)_config_opts_$(HOST)), $($(package)_config_opts_$(HOST)), $($(package)_config_opts_default))

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef