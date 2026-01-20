package=native_qt
include packages/qt_details.mk
$(package)_version=$(qt_details_version)
$(package)_download_path=$(qt_details_download_path)
$(package)_file_name=$(qt_details_qtbase_file_name)
$(package)_sha256_hash=$(qt_details_qtbase_sha256_hash)
$(package)_patches_path := $(qt_details_patches_path)
$(package)_patches := dont_hardcode_pwd.patch
$(package)_patches += qtbase_skip_tools.patch
$(package)_patches += rcc_hardcode_timestamp.patch
$(package)_patches += qttools_skip_dependencies.patch

$(package)_qttranslations_file_name=$(qt_details_qttranslations_file_name)
$(package)_qttranslations_sha256_hash=$(qt_details_qttranslations_sha256_hash)

$(package)_qttools_file_name=$(qt_details_qttools_file_name)
$(package)_qttools_sha256_hash=$(qt_details_qttools_sha256_hash)

$(package)_extra_sources := $($(package)_qttranslations_file_name)
$(package)_extra_sources += $($(package)_qttools_file_name)

$(package)_top_download_path=$(qt_details_top_download_path)
$(package)_top_cmakelists_file_name=$(qt_details_top_cmakelists_file_name)
$(package)_top_cmakelists_download_file=$(qt_details_top_cmakelists_download_file)
$(package)_top_cmakelists_sha256_hash=$(qt_details_top_cmakelists_sha256_hash)
$(package)_top_cmake_download_path=$(qt_details_top_cmake_download_path)
$(package)_top_cmake_ecmoptionaladdsubdirectory_file_name=$(qt_details_top_cmake_ecmoptionaladdsubdirectory_file_name)
$(package)_top_cmake_ecmoptionaladdsubdirectory_download_file=$(qt_details_top_cmake_ecmoptionaladdsubdirectory_download_file)
$(package)_top_cmake_ecmoptionaladdsubdirectory_sha256_hash=$(qt_details_top_cmake_ecmoptionaladdsubdirectory_sha256_hash)
$(package)_top_cmake_qttoplevelhelpers_file_name=$(qt_details_top_cmake_qttoplevelhelpers_file_name)
$(package)_top_cmake_qttoplevelhelpers_download_file=$(qt_details_top_cmake_qttoplevelhelpers_download_file)
$(package)_top_cmake_qttoplevelhelpers_sha256_hash=$(qt_details_top_cmake_qttoplevelhelpers_sha256_hash)

$(package)_extra_sources += $($(package)_top_cmakelists_file_name)-$($(package)_version)
$(package)_extra_sources += $($(package)_top_cmake_ecmoptionaladdsubdirectory_file_name)-$($(package)_version)
$(package)_extra_sources += $($(package)_top_cmake_qttoplevelhelpers_file_name)-$($(package)_version)

define $(package)_set_vars
# Build options.
$(package)_config_opts := -release
$(package)_config_opts += -make tools
$(package)_config_opts += -no-pkg-config
$(package)_config_opts += -no-reduce-relocations
$(package)_config_opts += -no-use-gold-linker
$(package)_config_opts += -prefix $(host_prefix)
$(package)_config_opts += -static

# Modules.
$(package)_config_opts += -no-feature-concurrent
$(package)_config_opts += -no-feature-network
$(package)_config_opts += -no-feature-printsupport
$(package)_config_opts += -no-feature-sql
$(package)_config_opts += -no-feature-testlib
$(package)_config_opts += -no-feature-xml
$(package)_config_opts += -no-gui
$(package)_config_opts += -no-widgets

$(package)_config_opts += -no-glib
$(package)_config_opts += -no-icu
$(package)_config_opts += -no-libudev
$(package)_config_opts += -no-openssl
$(package)_config_opts += -no-zstd
$(package)_config_opts += -qt-pcre
$(package)_config_opts += -qt-zlib
$(package)_config_opts += -no-feature-backtrace
$(package)_config_opts += -no-feature-permissions
$(package)_config_opts += -no-feature-process
$(package)_config_opts += -no-feature-settings

# Core tools.
$(package)_config_opts += -no-feature-androiddeployqt
$(package)_config_opts += -no-feature-macdeployqt
$(package)_config_opts += -no-feature-windeployqt
$(package)_config_opts += -no-feature-qmake

# Qt Tools module.
$(package)_config_opts += -feature-linguist
$(package)_config_opts += -no-feature-assistant
$(package)_config_opts += -no-feature-clang
$(package)_config_opts += -no-feature-clangcpp
$(package)_config_opts += -no-feature-designer
$(package)_config_opts += -no-feature-pixeltool
$(package)_config_opts += -no-feature-qdoc
$(package)_config_opts += -no-feature-qtattributionsscanner
$(package)_config_opts += -no-feature-qtdiag
$(package)_config_opts += -no-feature-qtplugininfo

$(package)_config_env := CC="$$(build_CC)"
$(package)_config_env += CXX="$$(build_CXX)"
ifeq ($(build_os),darwin)
$(package)_config_env += OBJC="$$(build_CC)"
$(package)_config_env += OBJCXX="$$(build_CXX)"
endif

$(package)_cmake_opts := -DCMAKE_EXE_LINKER_FLAGS="$$(build_LDFLAGS)"
ifneq ($(V),)
$(package)_cmake_opts += --log-level=STATUS
endif
endef

define $(package)_fetch_cmds
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_download_file),$($(package)_file_name),$($(package)_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qttranslations_file_name),$($(package)_qttranslations_file_name),$($(package)_qttranslations_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qttools_file_name),$($(package)_qttools_file_name),$($(package)_qttools_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_top_download_path),$($(package)_top_cmakelists_download_file),$($(package)_top_cmakelists_file_name)-$($(package)_version),$($(package)_top_cmakelists_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_top_cmake_download_path),$($(package)_top_cmake_ecmoptionaladdsubdirectory_download_file),$($(package)_top_cmake_ecmoptionaladdsubdirectory_file_name)-$($(package)_version),$($(package)_top_cmake_ecmoptionaladdsubdirectory_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_top_cmake_download_path),$($(package)_top_cmake_qttoplevelhelpers_download_file),$($(package)_top_cmake_qttoplevelhelpers_file_name)-$($(package)_version),$($(package)_top_cmake_qttoplevelhelpers_sha256_hash))
endef

define $(package)_extract_cmds
  mkdir -p $($(package)_extract_dir) && \
  echo "$($(package)_sha256_hash)  $($(package)_source)" > $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qttranslations_sha256_hash)  $($(package)_source_dir)/$($(package)_qttranslations_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qttools_sha256_hash)  $($(package)_source_dir)/$($(package)_qttools_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_top_cmakelists_sha256_hash)  $($(package)_source_dir)/$($(package)_top_cmakelists_file_name)-$($(package)_version)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_top_cmake_ecmoptionaladdsubdirectory_sha256_hash)  $($(package)_source_dir)/$($(package)_top_cmake_ecmoptionaladdsubdirectory_file_name)-$($(package)_version)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_top_cmake_qttoplevelhelpers_sha256_hash)  $($(package)_source_dir)/$($(package)_top_cmake_qttoplevelhelpers_file_name)-$($(package)_version)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  $(build_SHA256SUM) -c $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  mkdir qtbase && \
  $(build_TAR) --no-same-owner --strip-components=1 -xf $($(package)_source) -C qtbase && \
  mkdir qttranslations && \
  $(build_TAR) --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qttranslations_file_name) -C qttranslations && \
  mkdir qttools && \
  $(build_TAR) --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qttools_file_name) -C qttools && \
  cp $($(package)_source_dir)/$($(package)_top_cmakelists_file_name)-$($(package)_version) ./$($(package)_top_cmakelists_file_name) && \
  mkdir cmake && \
  cp $($(package)_source_dir)/$($(package)_top_cmake_ecmoptionaladdsubdirectory_file_name)-$($(package)_version) cmake/$($(package)_top_cmake_ecmoptionaladdsubdirectory_file_name) && \
  cp $($(package)_source_dir)/$($(package)_top_cmake_qttoplevelhelpers_file_name)-$($(package)_version) cmake/$($(package)_top_cmake_qttoplevelhelpers_file_name)
endef

define $(package)_preprocess_cmds
  patch -p1 -i $($(package)_patch_dir)/dont_hardcode_pwd.patch && \
  patch -p1 -i $($(package)_patch_dir)/qtbase_skip_tools.patch && \
  patch -p1 -i $($(package)_patch_dir)/rcc_hardcode_timestamp.patch && \
  patch -p1 -i $($(package)_patch_dir)/qttools_skip_dependencies.patch
endef

define $(package)_config_cmds
  cd qtbase && \
  ./configure -top-level $($(package)_config_opts) -- $($(package)_cmake_opts)
endef

define $(package)_build_cmds
  cmake --build . -- $$(filter -j%,$$(MAKEFLAGS))
endef

define $(package)_stage_cmds
  cmake --install . --prefix $($(package)_staging_prefix_dir) --strip
endef

define $(package)_postprocess_cmds
  rm -rf doc/ && \
  mv translations/ ..
endef
