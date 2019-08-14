package=libxcb
$(package)_version=1.10
$(package)_download_path=http://xcb.freedesktop.org/dist
$(package)_file_name=$(package)-$($(package)_version).tar.bz2
$(package)_sha256_hash=98d9ab05b636dd088603b64229dd1ab2d2cc02ab807892e107d674f9c3f2d5b5
$(package)_dependencies=xcb_proto libXau

define $(package)_set_vars
$(package)_config_opts=--disable-static
# Because we pass -qt-xcb to Qt, it will compile in a set of xcb helper libraries and extensions,
# so we skip building all of the extensions here.
# More info is available from: https://doc.qt.io/qt-5.9/linux-requirements.html
$(package)_config_opts += --disable-composite --disable-damage --disable-dpms
$(package)_config_opts += --disable-dri2 --disable-dri3 --disable-glx
$(package)_config_opts += --disable-present --disable-randr --disable-record
$(package)_config_opts += --disable-render --disable-resource --disable-screensaver
$(package)_config_opts += --disable-shape --disable-shm --disable-sync
$(package)_config_opts += --disable-xevie --disable-xfixes --disable-xfree86-dri
$(package)_config_opts += --disable-xinerama --disable-xinput --disable-xkb
$(package)_config_opts += --disable-xprint --disable-selinux --disable-xtest
$(package)_config_opts += --disable-xv --disable-xvmc
endef

define $(package)_preprocess_cmds
  sed "s/pthread-stubs//" -i configure
endef

# Don't install xcb headers to the default path in order to work around a qt
# build issue: https://bugreports.qt.io/browse/QTBUG-34748
# When using qt's internal libxcb, it may end up finding the real headers in
# depends staging. Use a non-default path to avoid that.

define $(package)_config_cmds
  $($(package)_autoconf) --includedir=$(host_prefix)/include/xcb-shared
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm -rf share/man share/doc
endef
