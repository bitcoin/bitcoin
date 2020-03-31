PACKAGE=qtwinextras
$(package)_version=$(qt_version)
$(package)_download_path=https://download.qt.io/archive/qt/5.12/5.12.11/submodules
$(package)_suffix=$(qt_suffix)
$(package)_file_name=$(qt_file_name)
$(package)_sha256_hash=$(qt_sha256_hash)
$(package)_dependencies=qt
$(package)_build_subdir=qtbase

$(package)_qtwinextras_file_name=qtwinextras-$(qt_suffix)
$(package)_qtwinextras_sha256_hash=69b1abc1fa448c85e48599f1adc59c2234cb8b49201126c007fac8efa00b9327

$(package)_extra_sources += $($(package)_qtwinextras_file_name)

$(package)_set_vars=$(qt_set_vars)

define $(package)_fetch_cmds
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_download_file),$($(package)_file_name),$($(package)_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qtwinextras_file_name),$($(package)_qtwinextras_file_name),$($(package)_qtwinextras_sha256_hash))
endef

define $(package)_extract_cmds
  mkdir -p $($(package)_extract_dir) && \
  echo "$($(package)_sha256_hash)  $($(package)_source)" > $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qtwinextras_sha256_hash)  $($(package)_source_dir)/$($(package)_qtwinextras_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  $(build_SHA256SUM) -c $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  mkdir qtbase && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source) -C qtbase && \
  mkdir qtwinextras && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qtwinextras_file_name) -C qtwinextras
endef

define $(package)_preprocess_cmds
  sed -i.old -e '/qwinjumplist/d' qtwinextras/src/winextras/winextras.pro && \
  patch -p1 -i $(PATCHES_PATH)/qt/dont_hardcode_pwd.patch && \
  patch -p1 -i $(PATCHES_PATH)/qt/fix_qt_pkgconfig.patch &&\
  echo "!host_build: QMAKE_CFLAGS     += $($(package)_cflags) $($(package)_cppflags)" >> qtbase/mkspecs/common/gcc-base.conf && \
  echo "!host_build: QMAKE_CXXFLAGS   += $($(package)_cxxflags) $($(package)_cppflags)" >> qtbase/mkspecs/common/gcc-base.conf && \
  echo "!host_build: QMAKE_LFLAGS     += $($(package)_ldflags)" >> qtbase/mkspecs/common/gcc-base.conf && \
  echo "QMAKE_LINK_OBJECT_MAX = 10" >> qtbase/mkspecs/win32-g++/qmake.conf &&\
  echo "QMAKE_LINK_OBJECT_SCRIPT = object_script" >> qtbase/mkspecs/win32-g++/qmake.conf &&\
  sed -i.old "s|QMAKE_CFLAGS           += |!host_build: QMAKE_CFLAGS            = $($(package)_cflags) $($(package)_cppflags) |" qtbase/mkspecs/win32-g++/qmake.conf && \
  sed -i.old "s|QMAKE_CXXFLAGS         += |!host_build: QMAKE_CXXFLAGS            = $($(package)_cxxflags) $($(package)_cppflags) |" qtbase/mkspecs/win32-g++/qmake.conf && \
  sed -i.old "0,/^QMAKE_LFLAGS_/s|^QMAKE_LFLAGS_|!host_build: QMAKE_LFLAGS            = $($(package)_ldflags)\n&|" qtbase/mkspecs/win32-g++/qmake.conf && \
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
  cd ../qtwinextras && ../qtbase/bin/qmake qtwinextras.pro -o Makefile && cd ..
endef

define $(package)_build_cmds
  ln -s $(host_prefix)/lib/libQt*.a lib/ &&\
  ln -s $(host_prefix)/native/bin/* bin/ &&\
  cd ../qtwinextras &&\
  $(MAKE) &&\
  sed -i 's/qtbase/qtwinextras/g' lib/pkgconfig/Qt5WinExtras.pc
endef

define $(package)_stage_cmds
  $(MAKE) -C ../qtwinextras INSTALL_ROOT=$($(package)_staging_dir) install_subtargets
endef

define $(package)_postprocess_cmds
  rm -rf native/mkspecs/ native/lib/ lib/cmake/ && \
  rm -f lib/lib*.la lib/*.prl plugins/*/*.prl
endef
