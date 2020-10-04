package=curl
$(package)_version=7.72.0
$(package)_dependencies=gnutls
$(package)_download_path=https://curl.haxx.se/download
$(package)_file_name=curl-$($(package)_version).tar.gz
$(package)_sha256_hash=d4d5899a3868fbb6ae1856c3e55a32ce35913de3956d1973caccd37bd0174fa2

define $(package)_set_vars
  # default settings
  $(package)_config_opts_default=LIBS="-lnettle -lhogweed -lgmp" --without-nghttp2 --disable-shared --enable-static --disable-ftp --without-ssl --with-gnutls="$(PREFIX_DIR)" --disable-ntlm-wb --disable-file --disable-ldap --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --enable-proxy --without-ca-path --without-ca-bundle --with-ca-fallback --disable-telnet --enable-threaded-resolver

  # mingw specific settings
  $(package)_config_opts_mingw32=LIBS="-lcrypt32 -lnettle -lhogweed -lgmp" -DCURL_STATIC_LIB -static" --disable-ftp --without-ssl --with-gnutls="$(PREFIX_DIR)" --disable-ntlm-wb --disable-file --disable-ldap --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --enable-proxy --without-ca-path --without-ca-bundle --with-ca-fallback --disable-telnet

  $(package)_config_opts_x86_64-w64-mingw32=$($(package)_config_opts_mingw32)
  $(package)_config_opts_i686-w64-mingw32=$($(package)_config_opts_mingw32)

  # darwin specific settings
  $(package)_config_opts_darwin=--with-sysroot="$(DARWIN_SDK_PATH)" --disable-ftp --without-gnutls --with-darwinssl --without-ssl --disable-ntlm-wb --disable-file --disable-ldap --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --enable-proxy --disable-telnet --enable-threaded-resolver
  $(package)_config_opts_x86_64-apple-darwin11=$($(package)_config_opts_darwin)

  # set settings based on host
  $(package)_config_opts = $(if $($(package)_config_opts_$(HOST)), $($(package)_config_opts_$(HOST)), $($(package)_config_opts_default))
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