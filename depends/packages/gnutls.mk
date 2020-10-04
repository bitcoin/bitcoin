package=gnutls
$(package)_version=3.5.8
$(package)_dependencies=nettle gmp
$(package)_download_path=https://www.gnupg.org/ftp/gcrypt/gnutls/v3.5
$(package)_file_name=$(package)-$($(package)_version).tar.xz
$(package)_sha256_hash=0e97f243ae72b70307d684b84c7fe679385aa7a7a0e37e5be810193dcc17d4ff
define $(package)_set_vars
  # default settings
  $(package)_config_env_default=NETTLE_CFLAGS="-static" GMP_CFLAGS="-static" PKG_CONFIG_LIBDIR="$(PREFIX_DIR)/lib/pkgconfig" CFLAGS="-I$(PREFIX_DIR)include -L$(PREFIX_DIR)lib -static"
  $(package)_config_opts_default=--disable-shared --enable-static --with-included-libtasn1 --with-included-unistring --enable-local-libopts --disable-non-suiteb-curves --disable-doc --without-p11-kit

  # darwin specific settings
  $(package)_config_env_darwin=PKG_CONFIG_LIBDIR="$(PREFIX_DIR)/lib/pkgconfig"
  $(package)_config_env_x86_64-apple-darwin11=$($(package)_config_env_darwin)

  # 32 bit linux settings
  $(package)_config_env_i686-pc-linux-gnu=NETTLE_CFLAGS="-static" GMP_CFLAGS="-static" PKG_CONFIG_LIBDIR="$(PREFIX_DIR)/lib/pkgconfig" CFLAGS="-I$(PREFIX_DIR)include -L$(PREFIX_DIR)lib -static -m32" CXXFLAGS="-m32" LDFLAGS="-m32"

  # mingw dll specific settings
  ifeq ($(BUILD_DLL), 1)
  $(package)_config_env_x86_64-w64-mingw32=PKG_CONFIG_LIBDIR="$(PREFIX_DIR)/lib/pkgconfig" CFLAGS="-I$(PREFIX_DIR)include -L$(PREFIX_DIR)lib -D_WIN32_WINNT=0x0600 -DNCRYPT_PAD_PKCS1_FLAG=2 -DNCRYPT_SHA1_ALGORITHM=BCRYPT_SHA1_ALGORITHM -DNCRYPT_SHA256_ALGORITHM=BCRYPT_SHA256_ALGORITHM -DCERT_NCRYPT_KEY_HANDLE_TRANSFER_PROP_ID=99"
  endif

  # set settings based on host
  $(package)_config_env = $(if $($(package)_config_env_$(HOST)), $($(package)_config_env_$(HOST)), $($(package)_config_env_default))
  $(package)_config_opts = $(if $($(package)_config_opts_$(HOST)), $($(package)_config_opts_$(HOST)), $($(package)_config_opts_default))
endef

define $(package)_config_cmds
  ./bootstrap && ./configure $($(package)_config_opts)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef