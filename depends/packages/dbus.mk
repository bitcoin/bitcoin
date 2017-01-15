package=dbus
$(package)_version=1.10.14
$(package)_download_path=http://dbus.freedesktop.org/releases/dbus
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=23238f70353e38ce5ca183ebc9525c0d97ac00ef640ad29cf794782af6e6a083
$(package)_dependencies=expat

define $(package)_set_vars
  $(package)_config_opts=--disable-tests --disable-doxygen-docs --disable-xml-docs --disable-static --without-x
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE) -C dbus libdbus-1.la
endef

define $(package)_stage_cmds
  $(MAKE) -C dbus DESTDIR=$($(package)_staging_dir) install-libLTLIBRARIES install-dbusincludeHEADERS install-nodist_dbusarchincludeHEADERS && \
  $(MAKE) DESTDIR=$($(package)_staging_dir) install-pkgconfigDATA
endef
