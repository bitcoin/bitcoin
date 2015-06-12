PACKAGE=qt
$(package)_version=5.2.1
$(package)_download_path=http://download.qt-project.org/official_releases/qt/5.2/$($(package)_version)/single
$(package)_file_name=$(package)-everywhere-opensource-src-$($(package)_version).tar.gz
$(package)_sha256_hash=84e924181d4ad6db00239d87250cc89868484a14841f77fb85ab1f1dbdcd7da1
$(package)_dependencies=openssl
$(package)_linux_dependencies=freetype fontconfig dbus libxcb libX11 xproto libXext
$(package)_build_subdir=qtbase
$(package)_qt_libs=corelib network widgets gui plugins testlib
$(package)_patches=mac-qmake.conf fix-xcb-include-order.patch qt5-tablet-osx.patch qt5-yosemite.patch

define $(package)_set_vars
$(package)_config_opts_release = -release
$(package)_config_opts_debug   = -debug
$(package)_config_opts += -opensource -confirm-license
$(package)_config_opts += -no-audio-backend
$(package)_config_opts += -no-glib
$(package)_config_opts += -no-icu
$(package)_config_opts += -no-cups
$(package)_config_opts += -no-iconv
$(package)_config_opts += -no-gif
$(package)_config_opts += -no-freetype
$(package)_config_opts += -no-nis
$(package)_config_opts += -no-pch
$(package)_config_opts += -no-feature-style-plastique
$(package)_config_opts += -no-qml-debug
$(package)_config_opts += -nomake examples
$(package)_config_opts += -nomake tests
$(package)_config_opts += -no-feature-style-cde
$(package)_config_opts += -no-feature-style-s60
$(package)_config_opts += -no-feature-style-motif
$(package)_config_opts += -no-feature-style-windowsmobile
$(package)_config_opts += -no-feature-style-windowsce
$(package)_config_opts += -no-feature-style-cleanlooks
$(package)_config_opts += -no-sql-db2
$(package)_config_opts += -no-sql-ibase
$(package)_config_opts += -no-sql-oci
$(package)_config_opts += -no-sql-tds
$(package)_config_opts += -no-sql-mysql
$(package)_config_opts += -no-sql-odbc
$(package)_config_opts += -no-sql-psql
$(package)_config_opts += -no-sql-sqlite
$(package)_config_opts += -no-sql-sqlite2
$(package)_config_opts += -skip qtsvg
$(package)_config_opts += -skip qtwebkit
$(package)_config_opts += -skip qtwebkit-examples
$(package)_config_opts += -skip qtserialport
$(package)_config_opts += -skip qtdeclarative
$(package)_config_opts += -skip qtmultimedia
$(package)_config_opts += -skip qtimageformats
$(package)_config_opts += -skip qtx11extras
$(package)_config_opts += -skip qtlocation
$(package)_config_opts += -skip qtsensors
$(package)_config_opts += -skip qtquick1
$(package)_config_opts += -skip qtquickcontrols
$(package)_config_opts += -skip qtactiveqt
$(package)_config_opts += -skip qtconnectivity
$(package)_config_opts += -skip qtmacextras
$(package)_config_opts += -skip qtwinextras
$(package)_config_opts += -skip qtxmlpatterns
$(package)_config_opts += -skip qtscript
$(package)_config_opts += -skip qtdoc
$(package)_config_opts += -prefix $(host_prefix)
$(package)_config_opts += -bindir $(build_prefix)/bin
$(package)_config_opts += -no-c++11
$(package)_config_opts += -openssl-linked
$(package)_config_opts += -v
$(package)_config_opts += -static
$(package)_config_opts += -silent
$(package)_config_opts += -pkg-config
$(package)_config_opts += -qt-libpng
$(package)_config_opts += -qt-libjpeg
$(package)_config_opts += -qt-zlib
$(package)_config_opts += -qt-pcre

ifneq ($(build_os),darwin)
$(package)_config_opts_darwin = -xplatform macx-clang-linux
$(package)_config_opts_darwin += -device-option MAC_SDK_PATH=$(OSX_SDK)
$(package)_config_opts_darwin += -device-option CROSS_COMPILE="$(host)-"
$(package)_config_opts_darwin += -device-option MAC_MIN_VERSION=$(OSX_MIN_VERSION)
$(package)_config_opts_darwin += -device-option MAC_TARGET=$(host)
$(package)_config_opts_darwin += -device-option MAC_LD64_VERSION=$(LD64_VERSION)
endif

$(package)_config_opts_linux  = -qt-xkbcommon
$(package)_config_opts_linux += -qt-xcb
$(package)_config_opts_linux += -no-eglfs
$(package)_config_opts_linux += -no-linuxfb
$(package)_config_opts_linux += -system-freetype
$(package)_config_opts_linux += -no-sm
$(package)_config_opts_linux += -fontconfig
$(package)_config_opts_linux += -no-xinput2
$(package)_config_opts_linux += -no-libudev
$(package)_config_opts_linux += -no-egl
$(package)_config_opts_linux += -no-opengl
$(package)_config_opts_arm_linux  = -platform linux-g++ -xplatform $(host)
$(package)_config_opts_i686_linux  = -xplatform linux-g++-32
$(package)_config_opts_mingw32  = -no-opengl -xplatform win32-g++ -device-option CROSS_COMPILE="$(host)-"
$(package)_build_env  = QT_RCC_TEST=1
endef

define $(package)_preprocess_cmds
  sed -i.old "s|updateqm.commands = \$$$$\$$$$LRELEASE|updateqm.commands = $($(package)_extract_dir)/qttools/bin/lrelease|" qttranslations/translations/translations.pro && \
  sed -i.old "s/src_plugins.depends = src_sql src_xml src_network/src_plugins.depends = src_xml src_network/" qtbase/src/src.pro && \
  sed -i.old "/XIproto.h/d" qtbase/src/plugins/platforms/xcb/qxcbxsettings.cpp && \
  sed -i.old 's/if \[ "$$$$XPLATFORM_MAC" = "yes" \]; then xspecvals=$$$$(macSDKify/if \[ "$$$$BUILD_ON_MAC" = "yes" \]; then xspecvals=$$$$(macSDKify/' qtbase/configure && \
  mkdir -p qtbase/mkspecs/macx-clang-linux &&\
  cp -f qtbase/mkspecs/macx-clang/Info.plist.lib qtbase/mkspecs/macx-clang-linux/ &&\
  cp -f qtbase/mkspecs/macx-clang/Info.plist.app qtbase/mkspecs/macx-clang-linux/ &&\
  cp -f qtbase/mkspecs/macx-clang/qplatformdefs.h qtbase/mkspecs/macx-clang-linux/ &&\
  cp -f $($(package)_patch_dir)/mac-qmake.conf qtbase/mkspecs/macx-clang-linux/qmake.conf && \
  patch -p1 < $($(package)_patch_dir)/fix-xcb-include-order.patch && \
  patch -p1 < $($(package)_patch_dir)/qt5-tablet-osx.patch && \
  patch -d qtbase -p1 < $($(package)_patch_dir)/qt5-yosemite.patch && \
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
