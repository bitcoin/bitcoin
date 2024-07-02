package=expat
$(package)_version=R_2_6_2
$(package)_download_path=https://github.com/libexpat/libexpat/archive/refs/tags/
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=fbd032683370d761ba68dba2566d3280a154f5290634172d60a79b24d366d9dc
$(package)_build_subdir=expat/build

# -D_DEFAULT_SOURCE defines __USE_MISC, which exposes additional
# definitions in endian.h, which are required for a working
# endianness check in configure when building with -flto.
define $(package)_set_vars
  $(package)_config_opts := -DCMAKE_BUILD_TYPE=RelWithDebInfo -DEXPAT_BUILD_TOOLS=OFF
  $(package)_config_opts += -DEXPAT_BUILD_EXAMPLES=OFF -DEXPAT_BUILD_TESTS=OFF
  $(package)_config_opts_debug := -DCMAKE_BUILD_TYPE=Debug
  $(package)_cppflags += -D_DEFAULT_SOURCE
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
  rm -rf share lib/cmake lib/*.la
endef
