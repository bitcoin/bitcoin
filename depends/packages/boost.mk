package=boost
$(package)_version=1.85.0
$(package)_download_path=https://github.com/boostorg/boost/releases/download/boost-$($(package)_version)
$(package)_file_name=boost-$($(package)_version)-cmake.tar.gz
$(package)_sha256_hash=ab9c9c4797384b0949dd676cf86b4f99553f8c148d767485aaac412af25183e6
$(package)_build_subdir=build

# This compiles a few libs unnecessarily because date_time and test don't have
# header-only build/install options

#install_name_tool is unused, so set it to the `true` binary so that CMake thinks it exists
define $(package)_set_vars
  $(package)_config_opts=-DBOOST_INCLUDE_LIBRARIES="date_time;multi_index;signals2;test" -DBOOST_INSTALL_LAYOUT=system
  $(package)_config_opts_darwin=-DCMAKE_INSTALL_NAME_TOOL=true
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
