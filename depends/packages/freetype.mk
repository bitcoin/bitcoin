package=freetype
$(package)_version=2.11.1
$(package)_download_path=https://download.savannah.gnu.org/releases/$(package)
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=f8db94d307e9c54961b39a1cc799a67d46681480696ed72ecf78d4473770f09b
$(package)_build_subdir=build
$(package)_patches += cmake_minimum.patch

define $(package)_set_vars
  $(package)_config_opts := -DCMAKE_BUILD_TYPE=None -DBUILD_SHARED_LIBS=TRUE
  $(package)_config_opts += -DCMAKE_DISABLE_FIND_PACKAGE_ZLIB=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_PNG=TRUE
  $(package)_config_opts += -DCMAKE_DISABLE_FIND_PACKAGE_HarfBuzz=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_BZip2=TRUE
  $(package)_config_opts += -DCMAKE_DISABLE_FIND_PACKAGE_BrotliDec=TRUE
endef

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/cmake_minimum.patch
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

