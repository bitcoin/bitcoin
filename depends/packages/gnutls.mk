package=gnutls
$(package)_version=3.5.8
$(package)_dependencies=nettle zlib
$(package)_download_path=https://www.gnupg.org/ftp/gcrypt/gnutls/v3.5/
$(package)_file_name=$(package)-$($(package)_version).tar.xz
$(package)_sha256_hash=0e97f243ae72b70307d684b84c7fe679385aa7a7a0e37e5be810193dcc17d4ff

# default settings
$(package)_config_opts=--disable-shared --enable-static --with-included-libtasn1 --with-included-unistring --enable-local-libopts --disable-non-suiteb-curves --disable-doc --disable-tools --without-p11-kit --disable-tests
$(package)_config_opts_linux=--with-pic

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