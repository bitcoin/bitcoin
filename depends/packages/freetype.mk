package=freetype
$(package)_version=2.11.0
$(package)_download_path=https://download.savannah.gnu.org/releases/$(package)
$(package)_file_name=$(package)-$($(package)_version).tar.xz
$(package)_sha256_hash=8bee39bd3968c4804b70614a0a3ad597299ad0e824bc8aad5ce8aaf48067bde7
$(package)_build_subdir=build

define $(package)_set_vars
  $(package)_config_opts := -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=true
  $(package)_config_opts += -DFT_DISABLE_ZLIB=ON -DFT_DISABLE_PNG=ON
  $(package)_config_opts += -DFT_DISABLE_HARFBUZZ=ON -DFT_DISABLE_BZIP2=ON
  $(package)_config_opts += -DFT_DISABLE_BROTLI=ON
  $(package)_config_opts_debug := -DCMAKE_BUILD_TYPE=Debug
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

define $(package)_postprocess_cmds
  rm -rf share/man
endef
