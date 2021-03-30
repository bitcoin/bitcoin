package=native_qtbase
$(package)_version=5.12.11
$(package)_download_path=https://download.qt.io/official_releases/qt/5.12/$($(package)_version)/submodules
$(package)_suffix=everywhere-src-$($(package)_version).tar.xz
$(package)_file_name=qtbase-$($(package)_suffix)
$(package)_sha256_hash=1c1b4e33137ca77881074c140d54c3c9747e845a31338cfe8680f171f0bc3a39
$(package)_patches=dont_hardcode_pwd.patch fix_limits_header.patch

$(package)_qttools_file_name=qttools-$($(package)_suffix)
$(package)_qttools_sha256_hash=98b2aaca230458f65996f3534fd471d2ffd038dd58ac997c0589c06dc2385b4f

$(package)_extra_sources += $($(package)_qttools_file_name)

define $(package)_set_vars
$(package)_config_opts_release = -release
$(package)_config_opts_release += -silent
$(package)_config_opts_debug = -debug
$(package)_config_opts_debug += -optimized-tools
$(package)_config_opts += -bindir $(build_prefix)/bin
$(package)_config_opts += -c++std c++1z
$(package)_config_opts += -confirm-license
$(package)_config_opts += -hostprefix $(build_prefix)
$(package)_config_opts += -make tools
$(package)_config_opts += -no-accessibility
$(package)_config_opts += -no-compile-examples
$(package)_config_opts += -no-cups
$(package)_config_opts += -no-dbus
$(package)_config_opts += -no-direct2d
$(package)_config_opts += -no-directfb
$(package)_config_opts += -no-egl
$(package)_config_opts += -no-eglfs
$(package)_config_opts += -no-evdev
$(package)_config_opts += -no-feature-bearermanagement
$(package)_config_opts += -no-feature-colordialog
$(package)_config_opts += -no-feature-concurrent
$(package)_config_opts += -no-feature-corewlan
$(package)_config_opts += -no-feature-dial
$(package)_config_opts += -no-feature-fontcombobox
$(package)_config_opts += -no-feature-ftp
$(package)_config_opts += -no-feature-gui
$(package)_config_opts += -no-feature-http
$(package)_config_opts += -no-feature-itemmodeltester
$(package)_config_opts += -no-feature-image_heuristic_mask
$(package)_config_opts += -no-feature-keysequenceedit
$(package)_config_opts += -no-feature-lcdnumber
$(package)_config_opts += -no-feature-network
$(package)_config_opts += -no-feature-networkdiskcache
$(package)_config_opts += -no-feature-networkproxy
$(package)_config_opts += -no-feature-pdf
$(package)_config_opts += -no-feature-printdialog
$(package)_config_opts += -no-feature-printer
$(package)_config_opts += -no-feature-printpreviewdialog
$(package)_config_opts += -no-feature-printpreviewwidget
$(package)_config_opts += -no-feature-sessionmanager
$(package)_config_opts += -no-feature-socks5
$(package)_config_opts += -no-feature-sql
$(package)_config_opts += -no-feature-sqlmodel
$(package)_config_opts += -no-feature-statemachine
$(package)_config_opts += -no-feature-syntaxhighlighter
$(package)_config_opts += -no-feature-testlib
$(package)_config_opts += -no-feature-textbrowser
$(package)_config_opts += -no-feature-textodfwriter
$(package)_config_opts += -no-feature-topleveldomain
$(package)_config_opts += -no-feature-udpsocket
$(package)_config_opts += -no-feature-undocommand
$(package)_config_opts += -no-feature-undogroup
$(package)_config_opts += -no-feature-undostack
$(package)_config_opts += -no-feature-undoview
$(package)_config_opts += -no-feature-vnc
$(package)_config_opts += -no-feature-vulkan
$(package)_config_opts += -no-feature-widgets
$(package)_config_opts += -no-feature-wizard
$(package)_config_opts += -no-feature-xmlstreamwriter
$(package)_config_opts += -no-freetype
$(package)_config_opts += -no-gbm
$(package)_config_opts += -no-gif
$(package)_config_opts += -no-glib
$(package)_config_opts += -no-gui
$(package)_config_opts += -no-harfbuzz
$(package)_config_opts += -no-icu
$(package)_config_opts += -no-ico
$(package)_config_opts += -no-iconv
$(package)_config_opts += -no-journald
$(package)_config_opts += -no-kms
$(package)_config_opts += -no-libjpeg
$(package)_config_opts += -no-libpng
$(package)_config_opts += -no-libproxy
$(package)_config_opts += -no-libudev
$(package)_config_opts += -no-linuxfb
$(package)_config_opts += -no-mirclient
$(package)_config_opts += -no-mtdev
$(package)_config_opts += -no-openssl
$(package)_config_opts += -no-opengl
$(package)_config_opts += -no-openvg
$(package)_config_opts += -no-qpa
$(package)_config_opts += -no-reduce-relocations
$(package)_config_opts += -no-sctp
$(package)_config_opts += -no-securetransport
$(package)_config_opts += -no-sql-db2
$(package)_config_opts += -no-sql-ibase
$(package)_config_opts += -no-sql-oci
$(package)_config_opts += -no-sql-tds
$(package)_config_opts += -no-sql-mysql
$(package)_config_opts += -no-sql-odbc
$(package)_config_opts += -no-sql-psql
$(package)_config_opts += -no-sql-sqlite
$(package)_config_opts += -no-sql-sqlite2
$(package)_config_opts += -no-system-proxies
$(package)_config_opts += -no-trace
$(package)_config_opts += -no-tslib
$(package)_config_opts += -no-use-gold-linker
$(package)_config_opts += -no-widgets
$(package)_config_opts += -no-xcb
$(package)_config_opts += -no-xcb-xlib
$(package)_config_opts += -no-xkbcommon
$(package)_config_opts += -nomake examples
$(package)_config_opts += -nomake tests
$(package)_config_opts += -opensource
$(package)_config_opts += -no-pkg-config
$(package)_config_opts += -prefix $(host_prefix)
$(package)_config_opts += -qt-pcre
$(package)_config_opts += -qt-zlib
$(package)_config_opts += -reduce-exports
$(package)_config_opts += -static
$(package)_config_opts += -v

# required for C++17 support on macOS
$(package)_config_opts_darwin = QMAKE_MACOSX_DEPLOYMENT_TARGET=$(OSX_MIN_VERSION)

# for macOS on Apple Silicon (ARM) see https://bugreports.qt.io/browse/QTBUG-85279
$(package)_config_opts_aarch64_darwin += -device-option QMAKE_APPLE_DEVICE_ARCHS=arm64
endef

define $(package)_fetch_cmds
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_download_file),$($(package)_file_name),$($(package)_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qttools_file_name),$($(package)_qttools_file_name),$($(package)_qttools_sha256_hash))
endef

define $(package)_extract_cmds
  mkdir -p $($(package)_extract_dir) && \
  echo "$($(package)_sha256_hash)  $($(package)_source)" > $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qttools_sha256_hash)  $($(package)_source_dir)/$($(package)_qttools_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  $(build_SHA256SUM) -c $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  mkdir qtbase && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source) -C qtbase && \
  mkdir qttools && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qttools_file_name) -C qttools
endef

define $(package)_preprocess_cmds
  patch -p1 -i $($(package)_patch_dir)/dont_hardcode_pwd.patch && \
  patch -p1 -i $($(package)_patch_dir)/fix_limits_header.patch
endef

define $(package)_config_cmds
  cd qtbase && \
  ./configure $($(package)_config_opts) && \
  cd .. && \
  $(MAKE) -C qtbase && \
  qtbase/bin/qmake -o qtbase/src/tools/uic/Makefile qtbase/src/tools/uic/uic.pro && \
  qtbase/bin/qmake -o qtbase/src/tools/qfloat16-tables/Makefile qtbase/src/tools/qfloat16-tables/qfloat16-tables.pro && \
  qtbase/bin/qmake -o qtbase/src/tools/qvkgen/Makefile qtbase/src/tools/qvkgen/qvkgen.pro && \
  qtbase/bin/qmake -o qtbase/qmake/Makefile qtbase/qmake/qmake-aux.pro && \
  qtbase/bin/qmake -o qttools/src/linguist/Makefile qttools/src/linguist/linguist.pro
endef

define $(package)_build_cmds
  $(MAKE) -C qtbase/src/tools/moc && \
  $(MAKE) -C qtbase/src/tools/qfloat16-tables && \
  $(MAKE) -C qtbase/src/tools/qlalr && \
  $(MAKE) -C qtbase/src/tools/qvkgen && \
  $(MAKE) -C qtbase/src/tools/rcc && \
  $(MAKE) -C qtbase/src/tools/uic && \
  $(MAKE) -C qttools/src/linguist
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/bin && \
  cp qtbase/bin/qmake $($(package)_staging_prefix_dir)/bin/ && \
  cp qtbase/bin/qfloat16-tables $($(package)_staging_prefix_dir)/bin/ && \
  $(MAKE) INSTALL_ROOT=$($(package)_staging_dir) -C qtbase/src/tools/moc install && \
  $(MAKE) INSTALL_ROOT=$($(package)_staging_dir) -C qtbase/src/tools/qlalr install && \
  $(MAKE) INSTALL_ROOT=$($(package)_staging_dir) -C qtbase/src/tools/qvkgen install && \
  $(MAKE) INSTALL_ROOT=$($(package)_staging_dir) -C qtbase/src/tools/rcc install && \
  $(MAKE) INSTALL_ROOT=$($(package)_staging_dir) -C qtbase/src/tools/uic install && \
  $(MAKE) INSTALL_ROOT=$($(package)_staging_dir) -C qttools/src/linguist/lrelease install && \
  $(MAKE) INSTALL_ROOT=$($(package)_staging_dir) -C qttools/src/linguist/lupdate install && \
  $(MAKE) INSTALL_ROOT=$($(package)_staging_dir) -C qttools/src/linguist/lconvert install
endef
