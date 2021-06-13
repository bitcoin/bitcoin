package=libxcb
$(package)_version=1.10
$(package)_download_path=https://xcb.freedesktop.org/dist
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=98d9ab05b636dd088603b64229dd1ab2d2cc02ab807892e107d674f9c3f2d5b5
$(package)_dependencies=xcb_proto libXau

define $(package)_set_vars
$(package)_config_opts=--disable-static --disable-build-docs --without-doxygen --without-launchd
$(package)_config_opts += --disable-dependency-tracking --enable-option-checking
# Because we pass -qt-xcb to Qt, it will compile in a set of xcb helper libraries and extensions,
# so we skip building all of the extensions here.
# More info is available from: https://doc.qt.io/qt-5.9/linux-requirements.html
$(package)_config_opts += --disable-composite --disable-damage --disable-dpms
$(package)_config_opts += --disable-dri2 --disable-dri3 --disable-glx
$(package)_config_opts += --disable-present --disable-randr --disable-record
$(package)_config_opts += --disable-render --disable-resource --disable-screensaver
$(package)_config_opts += --disable-shape --disable-sync
$(package)_config_opts += --disable-xevie --disable-xfixes --disable-xfree86-dri
$(package)_config_opts += --disable-xinerama --disable-xinput
$(package)_config_opts += --disable-xprint --disable-selinux --disable-xtest
$(package)_config_opts += --disable-xv --disable-xvmc
endef

define $(package)_preprocess_cmds
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub build-aux && \
  sed "s/pthread-stubs//" -i configure
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm -rf share lib/*.la
endef
packages/$(package).mk:: ;
