package=qrencode
$(package)_version=4.1.1
$(package)_download_path=https://github.com/fukuchi/libqrencode/archive/refs/tags/
$(package)_download_file=v$($(package)_version).tar.gz
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=5385bc1b8c2f20f3b91d258bf8ccc8cf62023935df2d2676b5b67049f31a049c
$(package)_patches=cmake_fixups.patch

define $(package)_set_vars
$(package)_config_opts := -DWITH_TOOLS=NO -DWITH_TESTS=NO -DGPROF=OFF -DCOVERAGE=OFF
$(package)_config_opts += -DCMAKE_DISABLE_FIND_PACKAGE_PNG=TRUE -DWITHOUT_PNG=ON
$(package)_config_opts += -DCMAKE_DISABLE_FIND_PACKAGE_ICONV=TRUE
$(package)_cflags += -Wno-int-conversion -Wno-implicit-function-declaration
endef

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/cmake_fixups.patch
endef


define $(package)_config_cmds
  $($(package)_cmake) -S . -B .
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm -rf share
endef
