
package=boost
$(package)_version=1.86.0
$(package)_download_path=https://github.com/boostorg/boost/releases/download/boost-$($(package)_version)
$(package)_file_name=boost-$($(package)_version)-cmake.tar.gz
$(package)_sha256_hash=c62ce6e64d34414864fef946363db91cea89c1b90360eabed0515f0eda74c75c
$(package)_build_subdir=build

# This compiles a few libs unnecessarily because date_time and test don't have
# header-only build/install options

define $(package)_set_vars
  $(package)_config_opts=-DBOOST_INCLUDE_LIBRARIES="date_time;multi_index;signals2;test" -DBOOST_INSTALL_LAYOUT=system
  $(package)_config_opts_darwin=-DCMAKE_PLATFORM_HAS_INSTALLNAME="FALSE"
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
  rm -rf lib/libboost*
endef
