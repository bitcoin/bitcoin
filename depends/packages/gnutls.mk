package=gnutls
$(package)_version=3.5.8
$(package)_dependencies=nettle
$(package)_download_path=https://www.gnupg.org/ftp/gcrypt/gnutls/v3.5
$(package)_file_name=$(package)-$($(package)_version).tar.xz
$(package)_sha256_hash=0e97f243ae72b70307d684b84c7fe679385aa7a7a0e37e5be810193dcc17d4ff


# default settings
$(package)_config_opts=NETTLE_CFLAGS="-static" GMP_CFLAGS="-static" --disable-shared --enable-static --with-included-libtasn1 --with-included-unistring --enable-local-libopts --disable-non-suiteb-curves --disable-doc --without-p11-kit

# mingw dll specific settings
ifeq ($(BUILD_DLL), 1)
$(package)_config_opts_x86_64_mingw32=PKG_CONFIG_LIBDIR="$(PREFIX_DIR)/lib/pkgconfig" CFLAGS="-I$(PREFIX_DIR)include -L$(PREFIX_DIR)lib -D_WIN32_WINNT=0x0600 -DNCRYPT_PAD_PKCS1_FLAG=2 -DNCRYPT_SHA1_ALGORITHM=BCRYPT_SHA1_ALGORITHM -DNCRYPT_SHA256_ALGORITHM=BCRYPT_SHA256_ALGORITHM -DCERT_NCRYPT_KEY_HANDLE_TRANSFER_PROP_ID=99"
endif

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef