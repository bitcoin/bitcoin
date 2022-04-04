package := native_wayland_scanner
$(package)_version := 1.19.0
$(package)_download_path := https://wayland.freedesktop.org/releases
$(package)_file_name := wayland-$($(package)_version).tar.xz
$(package)_sha256_hash := baccd902300d354581cd5ad3cc49daa4921d55fb416a5883e218750fef166d15

define $(package)_set_vars
  $(package)_config_opts := --enable-option-checking --disable-dependency-tracking
  $(package)_config_opts += --disable-libraries --disable-documentation
  $(package)_config_opts += --disable-dtd-validation
endef

define $(package)_config_cmds
  EXPAT_LIBS=$$$$(env -u PKG_CONFIG_LIBDIR PKG_CONFIG_PATH=$(SYSTEM_PKG_CONFIG_PATH) pkg-config --libs expat) \
  EXPAT_CFLAGS=$$$$(env -u PKG_CONFIG_LIBDIR PKG_CONFIG_PATH=$(SYSTEM_PKG_CONFIG_PATH) PKG_CONFIG_ALLOW_SYSTEM_CFLAGS=1 pkg-config --cflags expat) \
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
