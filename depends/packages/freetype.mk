package=freetype
$(package)_version=2.11.0
$(package)_download_path=https://download.savannah.gnu.org/releases/$(package)
$(package)_file_name=$(package)-$($(package)_version).tar.xz
$(package)_sha256_hash=8bee39bd3968c4804b70614a0a3ad597299ad0e824bc8aad5ce8aaf48067bde7
$(package)_build_subdir=build

define $(package)_set_vars
  $(package)_config_opts := -DCMAKE_BUILD_TYPE=None -DBUILD_SHARED_LIBS=TRUE
  $(package)_config_opts += -DCMAKE_DISABLE_FIND_PACKAGE_ZLIB=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_PNG=TRUE
  $(package)_config_opts += -DCMAKE_DISABLE_FIND_PACKAGE_HarfBuzz=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_BZip2=TRUE
  $(package)_config_opts += -DCMAKE_DISABLE_FIND_PACKAGE_BrotliDec=TRUE
endef

define $(package)_config_cmds
  $($(package)_cmake) -S .. -B .
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

