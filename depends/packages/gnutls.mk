package=gnutls
$(package)_version=3.6.15
$(package)_dependencies=nettle
$(package)_download_path=https://www.gnupg.org/ftp/gcrypt/gnutls/v3.6/
$(package)_file_name=$(package)-$($(package)_version).tar.xz
$(package)_sha256_hash=0ea8c3283de8d8335d7ae338ef27c53a916f15f382753b174c18b45ffd481558


# default settings
$(package)_config_opts=--disable-shared --enable-static --with-included-libtasn1 --with-included-unistring --disable-non-suiteb-curves --disable-doc --disable-tools --without-p11-kit --disable-tests
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