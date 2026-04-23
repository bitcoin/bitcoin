package=boost
$(package)_version = 1.91.0-1
$(package)_download_path = https://github.com/boostorg/boost/releases/download/boost-$($(package)_version)
$(package)_file_name = boost-$($(package)_version)-cmake.tar.gz
$(package)_sha256_hash = 8a82bd11a720c70923806c36ee5c26dbd2d630c1eaa1d8fad9a7bd5529908a26
$(package)_build_subdir = build

define $(package)_set_vars
  $(package)_config_opts = -DBOOST_INCLUDE_LIBRARIES="multi_index;test"
  $(package)_config_opts += -DBOOST_TEST_HEADERS_ONLY=ON
  $(package)_config_opts += -DBOOST_ENABLE_MPI=OFF
  $(package)_config_opts += -DBOOST_ENABLE_PYTHON=OFF
  $(package)_config_opts += -DBOOST_INSTALL_LAYOUT=system
  $(package)_config_opts += -DBUILD_TESTING=OFF
  $(package)_config_opts += -DCMAKE_DISABLE_FIND_PACKAGE_ICU=ON
  # Install to a unique path to prevent accidental inclusion via other dependencies' -I flags.
  $(package)_config_opts += -DCMAKE_INSTALL_INCLUDEDIR=$(package)/include
endef

define $(package)_config_cmds
  $($(package)_cmake) -S .. -B .
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm -rf share
endef
