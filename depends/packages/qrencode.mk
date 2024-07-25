package=qrencode
$(package)_version=4.1.1
$(package)_download_path=https://fukuchi.org/works/qrencode/
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=e455d9732f8041cf5b9c388e345a641fd15707860f928e94507b1961256a6923
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
