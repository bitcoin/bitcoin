package=gnutls
$(package)_version=3.7.0
$(package)_dependencies=nettle zlib
$(package)_download_path=https://www.gnupg.org/ftp/gcrypt/gnutls/v3.7/
$(package)_file_name=$(package)-$($(package)_version).tar.xz
$(package)_sha256_hash=49e2a22691d252c9f24a9829b293a8f359095bc5a818351f05f1c0a5188a1df8

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

define $(package)_postprocess_cmds
  rm lib/*.la
endef