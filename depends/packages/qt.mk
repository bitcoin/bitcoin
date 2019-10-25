PACKAGE=qt
$(package)_version=5.12.4
$(package)_download_path=https://download.qt.io/official_releases/qt/5.12/$($(package)_version)/submodules
$(package)_suffix=everywhere-src-$($(package)_version).tar.xz
$(package)_file_name=qtbase-$($(package)_suffix)
$(package)_sha256_hash=20fbc7efa54ff7db9552a7a2cdf9047b80253c1933c834f35b0bc5c1ae021195
$(package)_dependencies=zlib
$(package)_linux_dependencies=freetype fontconfig libxcb
$(package)_android_dependencies=freetype fontconfig libxcb
$(package)_mingw32_dependencies=openssl
$(package)_build_subdir=qtbase
$(package)_qt_libs=corelib network widgets gui plugins testlib
$(package)_patches=fix_qt_pkgconfig.patch mac-qmake.conf fix_configure_mac.patch fix_no_printer.patch fix_s390x_powerpc_mips_mipsel_architectures.patch android-qmake.conf android-base-head.patch fix_android_jni_static.patch

$(package)_qttranslations_file_name=qttranslations-$($(package)_suffix)
$(package)_qttranslations_sha256_hash=ab8dd55f5ca869cab51c3a6ce0888f854b96dc03c7f25d2bd3d2c50314ab60fb

$(package)_qttools_file_name=qttools-$($(package)_suffix)
$(package)_qttools_sha256_hash=3b0e353860a9c0cd4db9eeae5f94fef8811ed7d107e3e5e97e4a557f61bd6eb6

$(package)_qtcharts_file_name=qtcharts-$($(package)_suffix)
$(package)_qtcharts_sha256_hash=06ff68a80dc377847429cdd87d4e46465e1d6fbc417d52700a0a59d197669c9e

$(package)_qtandroidextras_file_name=qtandroidextras-$($(package)_suffix)
$(package)_qtandroidextras_sha256_hash=18e0dbd82920b0ca51b29172fc0ed1f2a923cb7c4fa8fb574595abc16ec3245e

$(package)_qtgraphicaleffects_file_name=qtgraphicaleffects-$($(package)_suffix)
$(package)_qtgraphicaleffects_sha256_hash=0bc38b168fa724411984525173d667aa47076c8cbd4eeb791d0da7fe4b9bdf73

$(package)_qtquickcontrols_file_name=qtquickcontrols-$($(package)_suffix)
$(package)_qtquickcontrols_sha256_hash=32d4c2505337c67b0bac26d7f565ec8fabdc616e61247e98674820769dda9858

$(package)_qtquickcontrols2_file_name=qtquickcontrols2-$($(package)_suffix)
$(package)_qtquickcontrols2_sha256_hash=9a447eed38bc8c7d7be7bc407317f58940377c077ddca74c9a641b1ee6200331

$(package)_qtxmlpatterns_file_name=qtxmlpatterns-$($(package)_suffix)
$(package)_qtxmlpatterns_sha256_hash=0bea1719bb948f65cbed4375cc3e997a6464f35d25b631bafbd7a3161f8f5666

$(package)_qtdeclarative_file_name=qtdeclarative-$($(package)_suffix)
$(package)_qtdeclarative_sha256_hash=614105ed73079d67d81b34fef31c9934c5e751342e4b2e0297128c8c301acda7

$(package)_extra_sources  = $($(package)_qttranslations_file_name)
$(package)_extra_sources += $($(package)_qttools_file_name)
$(package)_extra_sources += $($(package)_qtcharts_file_name)
$(package)_extra_sources += $($(package)_qtandroidextras_file_name)
$(package)_extra_sources += $($(package)_qtgraphicaleffects_file_name)
$(package)_extra_sources += $($(package)_qtquickcontrols_file_name)
$(package)_extra_sources += $($(package)_qtquickcontrols2_file_name)
$(package)_extra_sources += $($(package)_qtxmlpatterns_file_name)
$(package)_extra_sources += $($(package)_qtdeclarative_file_name)

define $(package)_set_vars
$(package)_config_opts_release = -release
$(package)_config_opts_debug = -debug
$(package)_config_opts += -bindir $(build_prefix)/bin
$(package)_config_opts += -c++std c++11
$(package)_config_opts += -confirm-license
$(package)_config_opts += -dbus-runtime
$(package)_config_opts += -hostprefix $(build_prefix)
$(package)_config_opts += -no-compile-examples
$(package)_config_opts += -no-cups
$(package)_config_opts += -no-egl
$(package)_config_opts += -no-eglfs
$(package)_config_opts += -no-freetype
$(package)_config_opts += -no-gif
$(package)_config_opts += -no-glib
$(package)_config_opts += -no-icu
$(package)_config_opts += -no-ico
$(package)_config_opts += -no-iconv
$(package)_config_opts += -no-kms
$(package)_config_opts += -no-linuxfb
$(package)_config_opts += -no-libjpeg
$(package)_config_opts += -no-libudev
$(package)_config_opts += -no-mtdev
$(package)_config_opts += -no-openvg
$(package)_config_opts += -no-reduce-relocations
$(package)_config_opts += -no-sql-db2
$(package)_config_opts += -no-sql-ibase
$(package)_config_opts += -no-sql-oci
$(package)_config_opts += -no-sql-tds
$(package)_config_opts += -no-sql-mysql
$(package)_config_opts += -no-sql-odbc
$(package)_config_opts += -no-sql-psql
$(package)_config_opts += -no-sql-sqlite
$(package)_config_opts += -no-sql-sqlite2
$(package)_config_opts += -no-use-gold-linker
$(package)_config_opts += -nomake examples
$(package)_config_opts += -nomake tests
$(package)_config_opts += -opensource
$(package)_config_opts += -optimized-qmake
$(package)_config_opts += -pch
$(package)_config_opts += -pkg-config
$(package)_config_opts += -prefix $(host_prefix)
$(package)_config_opts += -qt-libpng
$(package)_config_opts += -qt-pcre
$(package)_config_opts += -qt-harfbuzz
$(package)_config_opts += -system-zlib
$(package)_config_opts += -static
$(package)_config_opts += -silent
$(package)_config_opts += -v
$(package)_config_opts += -no-feature-bearermanagement
$(package)_config_opts += -no-feature-colordialog
$(package)_config_opts += -no-feature-commandlineparser
$(package)_config_opts += -no-feature-concurrent
$(package)_config_opts += -no-feature-dial
$(package)_config_opts += -no-feature-filesystemwatcher
$(package)_config_opts += -no-feature-fontcombobox
$(package)_config_opts += -no-feature-ftp
$(package)_config_opts += -no-feature-image_heuristic_mask
$(package)_config_opts += -no-feature-keysequenceedit
$(package)_config_opts += -no-feature-lcdnumber
$(package)_config_opts += -no-feature-pdf
$(package)_config_opts += -no-feature-printdialog
$(package)_config_opts += -no-feature-printer
$(package)_config_opts += -no-feature-printpreviewdialog
$(package)_config_opts += -no-feature-printpreviewwidget
$(package)_config_opts += -no-feature-sessionmanager
$(package)_config_opts += -no-feature-sql
$(package)_config_opts += -no-feature-syntaxhighlighter
$(package)_config_opts += -no-feature-textbrowser
$(package)_config_opts += -no-feature-textodfwriter
$(package)_config_opts += -no-feature-topleveldomain
$(package)_config_opts += -no-feature-udpsocket
$(package)_config_opts += -no-feature-undocommand
$(package)_config_opts += -no-feature-undogroup
$(package)_config_opts += -no-feature-undostack
$(package)_config_opts += -no-feature-undoview
$(package)_config_opts += -no-feature-vnc
$(package)_config_opts += -no-feature-wizard
$(package)_config_opts += -no-feature-xml

ifneq ($(build_os),darwin)
$(package)_config_opts_darwin = -xplatform macx-clang-linux
$(package)_config_opts_darwin += -device-option MAC_SDK_PATH=$(OSX_SDK)
$(package)_config_opts_darwin += -device-option MAC_SDK_VERSION=$(OSX_SDK_VERSION)
$(package)_config_opts_darwin += -device-option CROSS_COMPILE="$(host)-"
$(package)_config_opts_darwin += -device-option MAC_MIN_VERSION=$(OSX_MIN_VERSION)
$(package)_config_opts_darwin += -device-option MAC_TARGET=$(host)
endif

$(package)_config_opts_linux  = -qt-xcb
$(package)_config_opts_linux  = -xkbcommon
$(package)_config_opts_linux += -qpa xcb
$(package)_config_opts_linux += -no-xcb-xlib
$(package)_config_opts_linux += -no-feature-xlib
$(package)_config_opts_linux += -system-freetype
$(package)_config_opts_linux += -fontconfig
$(package)_config_opts_linux += -no-opengl
$(package)_config_opts_linux += -no-openssl
$(package)_config_opts_linux += -L $(host_prefix)/lib -I $(host_prefix)/include -static

$(package)_config_opts_arm_linux += -platform linux-g++ -xplatform talkcoin-linux-g++
$(package)_config_opts_i686_linux  = -xplatform linux-g++-32
$(package)_config_opts_x86_64_linux = -xplatform linux-g++-64
$(package)_config_opts_aarch64_linux = -xplatform linux-aarch64-gnu-g++
$(package)_config_opts_riscv64_linux = -platform linux-g++ -xplatform talkcoin-linux-g++
$(package)_config_opts_mingw32  = -no-opengl -openssl-linked -xplatform win32-g++ -device-option CROSS_COMPILE="$(host)-"
$(package)_config_opts_mingw32 += -L $(host_prefix)/lib -I $(host_prefix)/include

$(package)_config_opts_android = -xplatform android-clang
$(package)_config_opts_android += -android-sdk $(ANDROID_SDK)
$(package)_config_opts_android += -android-ndk $(ANDROID_NDK)
$(package)_config_opts_android += -android-ndk-platform android-$(ANDROID_API_LEVEL)
$(package)_config_opts_android += -device-option CROSS_COMPILE="$(host)-"
$(package)_config_opts_android += -qpa xcb
$(package)_config_opts_android += -qt-freetype
$(package)_config_opts_android += -no-fontconfig
$(package)_config_opts_android += -opengl es2
$(package)_config_opts_android += -egl
$(package)_config_opts_android += -L $(host_prefix)/lib -I $(host_prefix)/include -static

$(package)_config_opts_aarch64_android += -android-arch arm64-v8a
$(package)_config_opts_armv7a_android += -android-arch armeabi-v7a
$(package)_config_opts_x86_64_android += -android-arch x86_64
$(package)_config_opts_i686_android += -android-arch i686

$(package)_build_env  = QT_RCC_TEST=1
$(package)_build_env += QT_RCC_SOURCE_DATE_OVERRIDE=1
endef

define $(package)_fetch_cmds
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_download_file),$($(package)_file_name),$($(package)_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qttranslations_file_name),$($(package)_qttranslations_file_name),$($(package)_qttranslations_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qtcharts_file_name),$($(package)_qtcharts_file_name),$($(package)_qtcharts_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qtandroidextras_file_name),$($(package)_qtandroidextras_file_name),$($(package)_qtandroidextras_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qtgraphicaleffects_file_name),$($(package)_qtgraphicaleffects_file_name),$($(package)_qtgraphicaleffects_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qttools_file_name),$($(package)_qttools_file_name),$($(package)_qttools_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qtquickcontrols_file_name),$($(package)_qtquickcontrols_file_name),$($(package)_qtquickcontrols_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qtquickcontrols2_file_name),$($(package)_qtquickcontrols2_file_name),$($(package)_qtquickcontrols2_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qtxmlpatterns_file_name),$($(package)_qtxmlpatterns_file_name),$($(package)_qtxmlpatterns_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qtdeclarative_file_name),$($(package)_qtdeclarative_file_name),$($(package)_qtdeclarative_sha256_hash))
endef

define $(package)_extract_cmds
  mkdir -p $($(package)_extract_dir) && \
  echo "$($(package)_sha256_hash)  $($(package)_source)" > $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qttranslations_sha256_hash)  $($(package)_source_dir)/$($(package)_qttranslations_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qttools_sha256_hash)  $($(package)_source_dir)/$($(package)_qttools_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qtcharts_sha256_hash)  $($(package)_source_dir)/$($(package)_qtcharts_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qtandroidextras_sha256_hash)  $($(package)_source_dir)/$($(package)_qtandroidextras_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qtgraphicaleffects_sha256_hash)  $($(package)_source_dir)/$($(package)_qtgraphicaleffects_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qtquickcontrols_sha256_hash)  $($(package)_source_dir)/$($(package)_qtquickcontrols_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qtquickcontrols2_sha256_hash)  $($(package)_source_dir)/$($(package)_qtquickcontrols2_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qtxmlpatterns_sha256_hash)  $($(package)_source_dir)/$($(package)_qtxmlpatterns_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qtdeclarative_sha256_hash)  $($(package)_source_dir)/$($(package)_qtdeclarative_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  $(build_SHA256SUM) -c $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  mkdir qtbase && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source) -C qtbase && \
  mkdir qttranslations && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qttranslations_file_name) -C qttranslations && \
  mkdir qtdeclarative && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qtdeclarative_file_name) -C qtdeclarative && \
  mkdir qtcharts && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qtcharts_file_name) -C qtcharts && \
  mkdir qtandroidextras && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qtandroidextras_file_name) -C qtandroidextras && \
  mkdir qtgraphicaleffects && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qtgraphicaleffects_file_name) -C qtgraphicaleffects && \
  mkdir qtxmlpatterns && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qtxmlpatterns_file_name) -C qtxmlpatterns && \
  mkdir qtquickcontrols && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qtquickcontrols_file_name) -C qtquickcontrols && \
  mkdir qtquickcontrols2 && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qtquickcontrols2_file_name) -C qtquickcontrols2 && \
  mkdir qttools && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qttools_file_name) -C qttools
endef

define $(package)_preprocess_cmds
  sed -i.old "s|FT_Get_Font_Format|FT_Get_X11_Font_Format|" qtbase/src/platformsupport/fontdatabases/freetype/qfontengine_ft.cpp && \
  sed -i.old "s|updateqm.commands = \$$$$\$$$$LRELEASE|updateqm.commands = $($(package)_extract_dir)/qttools/bin/lrelease|" qttranslations/translations/translations.pro && \
  sed -i.old "/updateqm.depends =/d" qttranslations/translations/translations.pro && \
  sed -i.old "s/src_plugins.depends = src_sql src_network/src_plugins.depends = src_network/" qtbase/src/src.pro && \
  sed -i.old -e 's/if \[ "$$$$XPLATFORM_MAC" = "yes" \]; then xspecvals=$$$$(macSDKify/if \[ "$$$$BUILD_ON_MAC" = "yes" \]; then xspecvals=$$$$(macSDKify/' -e 's|/bin/pwd|pwd|' qtbase/configure && \
  mkdir -p qtbase/mkspecs/macx-clang-linux &&\
  cp -f qtbase/mkspecs/macx-clang/Info.plist.lib qtbase/mkspecs/macx-clang-linux/ &&\
  cp -f qtbase/mkspecs/macx-clang/Info.plist.app qtbase/mkspecs/macx-clang-linux/ &&\
  cp -f qtbase/mkspecs/macx-clang/qplatformdefs.h qtbase/mkspecs/macx-clang-linux/ &&\
  cp -f $($(package)_patch_dir)/android-qmake.conf qtbase/mkspecs/android-clang/qmake.conf &&\
  cp -f $($(package)_patch_dir)/mac-qmake.conf qtbase/mkspecs/macx-clang-linux/qmake.conf && \
  cp -r qtbase/mkspecs/linux-arm-gnueabi-g++ qtbase/mkspecs/talkcoin-linux-g++ && \
  sed -i.old "s/arm-linux-gnueabi-/$(host)-/g" qtbase/mkspecs/talkcoin-linux-g++/qmake.conf && \
  patch -p1 -i $($(package)_patch_dir)/fix_qt_pkgconfig.patch &&\
  patch -p1 -i $($(package)_patch_dir)/fix_configure_mac.patch &&\
  patch -p1 -i $($(package)_patch_dir)/fix_no_printer.patch &&\
  patch -p1 -i $($(package)_patch_dir)/android-base-head.patch &&\
  patch -p1 -i $($(package)_patch_dir)/fix_android_jni_static.patch &&\
  echo "!host_build: QMAKE_CFLAGS     += $($(package)_cflags) $($(package)_cppflags)" >> qtbase/mkspecs/common/gcc-base.conf && \
  echo "!host_build: QMAKE_CXXFLAGS   += $($(package)_cxxflags) $($(package)_cppflags)" >> qtbase/mkspecs/common/gcc-base.conf && \
  echo "!host_build: QMAKE_LFLAGS     += $($(package)_ldflags)" >> qtbase/mkspecs/common/gcc-base.conf && \
  echo "QMAKE_LINK_OBJECT_MAX = 10" >> qtbase/mkspecs/win32-g++/qmake.conf &&\
  echo "QMAKE_LINK_OBJECT_SCRIPT = object_script" >> qtbase/mkspecs/win32-g++/qmake.conf &&\
  sed -i.old "s|QMAKE_CFLAGS            = |!host_build: QMAKE_CFLAGS            = $($(package)_cflags) $($(package)_cppflags) |" qtbase/mkspecs/win32-g++/qmake.conf && \
  sed -i.old "s|QMAKE_LFLAGS            = |!host_build: QMAKE_LFLAGS            = $($(package)_ldflags) |" qtbase/mkspecs/win32-g++/qmake.conf && \
  sed -i.old "s|QMAKE_CXXFLAGS          = |!host_build: QMAKE_CXXFLAGS            = $($(package)_cxxflags) $($(package)_cppflags) |" qtbase/mkspecs/win32-g++/qmake.conf && \
  sed -i.old "s/LIBRARY_PATH/(CROSS_)?\0/g" qtbase/mkspecs/features/toolchain.prf
endef

define $(package)_config_cmds
  export PKG_CONFIG_SYSROOT_DIR=/ && \
  export PKG_CONFIG_LIBDIR=$(host_prefix)/lib/pkgconfig && \
  export PKG_CONFIG_PATH=$(host_prefix)/share/pkgconfig  && \
  ./configure $($(package)_config_opts) && \
  echo "host_build: QT_CONFIG ~= s/system-zlib/zlib" >> mkspecs/qconfig.pri && \
  echo "CONFIG += force_bootstrap" >> mkspecs/qconfig.pri && \
  $(MAKE) sub-src-clean && \
  cd ../qttranslations && ../qtbase/bin/qmake qttranslations.pro -o Makefile && \
  cd translations && ../../qtbase/bin/qmake translations.pro -o Makefile && cd ../.. && \
  cd qttools/src/linguist/lrelease/ && ../../../../qtbase/bin/qmake lrelease.pro -o Makefile && \
  cd ../lupdate/ && ../../../../qtbase/bin/qmake lupdate.pro -o Makefile && cd ../../../.. && \
  cd qtcharts && ../qtbase/bin/qmake qtcharts.pro -o Makefile && cd ../ && \
  cd qtandroidextras && ../qtbase/bin/qmake qtandroidextras.pro -o Makefile && cd ../ && \
  cd qtbase/src/tools/androiddeployqt/ && ../../../../qtbase/bin/qmake androiddeployqt.pro -o Makefile && cd ../../../../ && \
  cd qtbase/src/tools/qlalr/ && ../../../../qtbase/bin/qmake qlalr.pro -o Makefile && cd ../../../../ && \
  cd qtdeclarative && ../qtbase/bin/qmake qtdeclarative.pro -o Makefile && cd ../ && \
  cd qtgraphicaleffects && ../qtbase/bin/qmake qtgraphicaleffects.pro -o Makefile && cd ../ && \
  cd qtxmlpatterns && ../qtbase/bin/qmake qtxmlpatterns.pro -o Makefile && cd ../ && \
  cd qtquickcontrols && ../qtbase/bin/qmake qtquickcontrols.pro -o Makefile && cd ../ && \
  cd qtquickcontrols2 && ../qtbase/bin/qmake qtquickcontrols2.pro -o Makefile && cd ../
endef

define $(package)_build_cmds
  $(MAKE) -C src $(addprefix sub-,$($(package)_qt_libs)) && \
  $(MAKE) -C ../qttools/src/linguist/lrelease && \
  $(MAKE) -C ../qttools/src/linguist/lupdate && \
  $(MAKE) -C ../qttranslations && \
  $(MAKE) -C ../qtcharts && \
  $(MAKE) -C ../qtandroidextras && \
  $(MAKE) -C ../qtbase/src/tools/androiddeployqt && \
  $(MAKE) -C ../qtbase/src/tools/qlalr && \
  $(MAKE) -C ../qtdeclarative && \
  $(MAKE) -C ../qtgraphicaleffects && \
  $(MAKE) -C ../qtxmlpatterns && \
  $(MAKE) -C ../qtquickcontrols && \
  $(MAKE) -C ../qtquickcontrols2
endef

define $(package)_stage_cmds
  $(MAKE) -C src INSTALL_ROOT=$($(package)_staging_dir) $(addsuffix -install_subtargets,$(addprefix sub-,$($(package)_qt_libs))) && cd .. && \
  $(MAKE) -C qttools/src/linguist/lrelease INSTALL_ROOT=$($(package)_staging_dir) install_target && \
  $(MAKE) -C qttools/src/linguist/lupdate INSTALL_ROOT=$($(package)_staging_dir) install_target && \
  $(MAKE) -C qttranslations INSTALL_ROOT=$($(package)_staging_dir) install_subtargets && \
  $(MAKE) -C qtcharts INSTALL_ROOT=$($(package)_staging_dir) install_subtargets && \
  $(MAKE) -C qtandroidextras INSTALL_ROOT=$($(package)_staging_dir) install_subtargets && \
  $(MAKE) -C qtbase/src/tools/androiddeployqt INSTALL_ROOT=$($(package)_staging_dir) install_target && \
  $(MAKE) -C qtbase/src/tools/qlalr INSTALL_ROOT=$($(package)_staging_dir) install_target && \
  $(MAKE) -C qtdeclarative INSTALL_ROOT=$($(package)_staging_dir) install_subtargets && \
  $(MAKE) -C qtxmlpatterns INSTALL_ROOT=$($(package)_staging_dir) install_subtargets && \
  $(MAKE) -C qtgraphicaleffects INSTALL_ROOT=$($(package)_staging_dir) install && \
  $(MAKE) -C qtquickcontrols INSTALL_ROOT=$($(package)_staging_dir) install && \
  $(MAKE) -C qtquickcontrols2 INSTALL_ROOT=$($(package)_staging_dir) install && \
  if `test -f qtbase/src/plugins/platforms/xcb/xcb-static/libxcb-static.a`; then \
    cp qtbase/src/plugins/platforms/xcb/xcb-static/libxcb-static.a $($(package)_staging_prefix_dir)/lib; \
  fi
endef

define $(package)_postprocess_cmds
  rm -rf native/mkspecs/ native/lib/ lib/cmake/ && \
  rm -f lib/lib*.la lib/*.prl plugins/*/*.prl
endef
