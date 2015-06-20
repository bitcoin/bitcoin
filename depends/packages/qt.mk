PACKAGE=qt
$(package)_version=5.4.2
$(package)_download_path=http://download.qt.io/official_releases/qt/5.4/$($(package)_version)/submodules
qt_suffix=opensource-src-$($(package)_version).tar.gz
$(package)_file_name=qt5-$(qt_suffix)
$(package)_sha256_hash=165840bcb69f670070cc71247179b696a839917b0758f332b88b6aab45fac400
$(package)_dependencies=openssl
$(package)_linux_dependencies=freetype fontconfig dbus libxcb libX11 xproto libXext
$(package)_build_subdir=qtbase
$(package)_qt_libs=corelib network widgets gui plugins testlib
$(package)_patches=mac-qmake.conf fix-xcb-include-order.patch
qtbase_file_name=qtbase-$(qt_suffix)
qtbase_sha256_hash=93a6ce12f2020da7b8b81b5fb42d7804fcdf3194fbd2cae156e3d01e3669c18a
qttranslations_file_name=qttranslations-$(qt_suffix)
qttranslations_sha256_hash=e1929c49bc1877406d0905f3bfaef720a458d8d63df0ce5e44883c17bbe36ba6
qttools_file_name=qttools-$(qt_suffix)
qttools_sha256_hash=8e8c8a9f46afd6b78f464bd25e4ef09f290208e99a2a90c0c173602be9adfd2d
$(package)_extra_sources=$(qtbase_file_name) $(qttranslations_file_name) $(qttools_file_name)

define $(package)_set_vars
$(package)_config_opts_release = -release
$(package)_config_opts_debug   = -debug
$(package)_config_opts += -opensource -confirm-license \
    -no-audio-backend \
    -no-glib \
    -no-icu \
    -no-cups \
    -no-iconv \
    -no-gif \
    -no-freetype \
    -no-nis \
    -no-pch \
    -no-qml-debug \
    -nomake examples \
    -nomake tests \
    -no-feature-style-windowsmobile \
    -no-feature-style-windowsce \
    -no-sql-db2 \
    -no-sql-ibase \
    -no-sql-oci \
    -no-sql-tds \
    -no-sql-mysql \
    -no-sql-odbc \
    -no-sql-psql \
    -no-sql-sqlite \
    -no-sql-sqlite2 \
    -prefix $(host_prefix) \
    -bindir $(build_prefix)/bin \
    -no-c++11 \
    -no-reduce-relocations \
    -openssl-linked \
    -v \
    -static \
    -silent \
    -pkg-config \
    -qt-libpng \
    -qt-libjpeg \
    -qt-zlib \
    -qt-pcre

ifneq ($(build_os),darwin)
$(package)_config_opts_darwin = -xplatform macx-clang-linux \
    -device-option MAC_SDK_PATH=$(OSX_SDK) \
    -device-option CROSS_COMPILE="$(host)-" \
    -device-option MAC_MIN_VERSION=$(OSX_MIN_VERSION) \
    -device-option MAC_TARGET=$(host) \
    -device-option MAC_LD64_VERSION=$(LD64_VERSION)
endif

$(package)_config_opts_linux  = -qt-xkbcommon \
    -qt-xcb \
    -no-eglfs \
    -no-linuxfb \
    -system-freetype \
    -no-sm \
    -fontconfig \
    -no-xinput2 \
    -no-libudev \
    -no-egl \
    -no-opengl
$(package)_config_opts_arm_linux  = -platform linux-g++ -xplatform $(host)
$(package)_config_opts_i686_linux  = -xplatform linux-g++-32
$(package)_config_opts_mingw32  = -no-opengl -xplatform win32-g++ -device-option CROSS_COMPILE="$(host)-"
$(package)_build_env  = QT_RCC_TEST=1
endef

define $(package)_fetch_cmds
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_download_file),$($(package)_file_name),$($(package)_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$(qtbase_file_name),$(qtbase_file_name),$(qtbase_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$(qttranslations_file_name),$(qttranslations_file_name),$(qttranslations_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$(qttools_file_name),$(qttools_file_name),$(qttools_sha256_hash))
endef

define $(package)_extract_cmds
  mkdir -p $($(package)_extract_dir) && \
  echo "$($(package)_sha256_hash)  $($(package)_source)" > $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$(qttranslations_sha256_hash)  $($(package)_source_dir)/$(qttranslations_file_name)" > $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$(qttools_sha256_hash)  $($(package)_source_dir)/$(qttools_file_name)" > $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  $(build_SHA256SUM) -c $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  tar --strip-components=1 -xf $($(package)_source) && \
  mkdir qtbase && \
  tar --strip-components=1 -xf $($(package)_source_dir)/$(qtbase_file_name) -C qtbase && \
  mkdir qttranslations && \
  tar --strip-components=1 -xf $($(package)_source_dir)/$(qttranslations_file_name) -C qttranslations && \
  mkdir qttools && \
  tar --strip-components=1 -xf $($(package)_source_dir)/$(qttools_file_name) -C qttools
endef

define $(package)_preprocess_cmds
  sed -i.old "s|updateqm.commands = \$$$$\$$$$LRELEASE|updateqm.commands = $($(package)_extract_dir)/qttools/bin/lrelease|" qttranslations/translations/translations.pro && \
  sed -i.old "s/src_plugins.depends = src_sql src_xml src_network/src_plugins.depends = src_xml src_network/" qtbase/src/src.pro && \
  sed -i.old 's/if \[ "$$$$XPLATFORM_MAC" = "yes" \]; then xspecvals=$$$$(macSDKify/if \[ "$$$$BUILD_ON_MAC" = "yes" \]; then xspecvals=$$$$(macSDKify/' qtbase/configure && \
  mkdir -p qtbase/mkspecs/macx-clang-linux &&\
  cp -f qtbase/mkspecs/macx-clang/Info.plist.lib qtbase/mkspecs/macx-clang-linux/ &&\
  cp -f qtbase/mkspecs/macx-clang/Info.plist.app qtbase/mkspecs/macx-clang-linux/ &&\
  cp -f qtbase/mkspecs/macx-clang/qplatformdefs.h qtbase/mkspecs/macx-clang-linux/ &&\
  cp -f $($(package)_patch_dir)/mac-qmake.conf qtbase/mkspecs/macx-clang-linux/qmake.conf && \
  patch -p1 < $($(package)_patch_dir)/fix-xcb-include-order.patch && \
  echo "QMAKE_CFLAGS     += $($(package)_cflags) $($(package)_cppflags)" >> qtbase/mkspecs/common/gcc-base.conf && \
  echo "QMAKE_CXXFLAGS   += $($(package)_cxxflags) $($(package)_cppflags)" >> qtbase/mkspecs/common/gcc-base.conf && \
  echo "QMAKE_LFLAGS     += $($(package)_ldflags)" >> qtbase/mkspecs/common/gcc-base.conf && \
  sed -i.old "s|QMAKE_CFLAGS            = |QMAKE_CFLAGS            = $($(package)_cflags) $($(package)_cppflags) |" qtbase/mkspecs/win32-g++/qmake.conf && \
  sed -i.old "s|QMAKE_LFLAGS            = |QMAKE_LFLAGS            = $($(package)_ldflags) |" qtbase/mkspecs/win32-g++/qmake.conf && \
  sed -i.old "s|QMAKE_CXXFLAGS          = |QMAKE_CXXFLAGS            = $($(package)_cxxflags) $($(package)_cppflags) |" qtbase/mkspecs/win32-g++/qmake.conf
endef

define $(package)_config_cmds
  export PKG_CONFIG_SYSROOT_DIR=/ && \
  export PKG_CONFIG_LIBDIR=$(host_prefix)/lib/pkgconfig && \
  export PKG_CONFIG_PATH=$(host_prefix)/share/pkgconfig  && \
  export CPATH=$(host_prefix)/include && \
  ./configure $($(package)_config_opts) && \
  $(MAKE) sub-src-clean && \
  cd ../qttranslations && ../qtbase/bin/qmake qttranslations.pro -o Makefile && \
  cd translations && ../../qtbase/bin/qmake translations.pro -o Makefile && cd ../.. &&\
  cd qttools/src/linguist/lrelease/ && ../../../../qtbase/bin/qmake lrelease.pro -o Makefile
endef

define $(package)_build_cmds
  export CPATH=$(host_prefix)/include && \
  $(MAKE) -C src $(addprefix sub-,$($(package)_qt_libs)) && \
  $(MAKE) -C ../qttools/src/linguist/lrelease && \
  $(MAKE) -C ../qttranslations
endef

define $(package)_stage_cmds
  $(MAKE) -C src INSTALL_ROOT=$($(package)_staging_dir) $(addsuffix -install_subtargets,$(addprefix sub-,$($(package)_qt_libs))) && cd .. &&\
  $(MAKE) -C qttools/src/linguist/lrelease INSTALL_ROOT=$($(package)_staging_dir) install_target && \
  $(MAKE) -C qttranslations INSTALL_ROOT=$($(package)_staging_dir) install_subtargets && \
  if `test -f qtbase/src/plugins/platforms/xcb/xcb-static/libxcb-static.a`; then \
    cp qtbase/src/plugins/platforms/xcb/xcb-static/libxcb-static.a $($(package)_staging_prefix_dir)/lib; \
  fi
endef

define $(package)_postprocess_cmds
  rm -rf mkspecs/ lib/cmake/ && \
  rm lib/libQt5Bootstrap.a lib/lib*.la lib/*.prl plugins/*/*.prl
endef
