package=expat
$(package)_version=2.7.3
$(package)_download_path=https://github.com/libexpat/libexpat/releases/download/R_$(subst .,_,$($(package)_version))/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=821ac9710d2c073eaf13e1b1895a9c9aa66c1157a99635c639fbff65cdbdd732
$(package)_build_subdir=build

define $(package)_set_vars
  $(package)_config_opts := -DCMAKE_BUILD_TYPE=None -DEXPAT_BUILD_TOOLS=OFF
  $(package)_config_opts += -DEXPAT_BUILD_EXAMPLES=OFF -DEXPAT_BUILD_TESTS=OFF
  $(package)_config_opts += -DBUILD_SHARED_LIBS=OFF
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
  rm -rf share lib/cmake
endef
