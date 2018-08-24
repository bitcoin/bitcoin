package=libpng
$(package)_version=1.6.37
$(package)_download_path=https://downloads.sourceforge.net/project/libpng/libpng16/$($(package)_version)/
$(package)_file_name=$(package)-$($(package)_version).tar.xz
$(package)_sha256_hash=505e70834d35383537b6491e7ae8641f1a4bed1876dbfe361201fc80868d88ca
$(package)_dependencies=zlib

define $(package)_set_vars
  $(package)_config_opts=--enable-static --disable-shared
  $(package)_config_opts += --disable-dependency-tracking
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE) $($(package)_build_opts) PNG_COPTS='-fPIC'
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install $($(package)_build_opts)
endef

