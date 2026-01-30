package=libxcb_util_keysyms
$(package)_version=0.4.1
$(package)_download_path=https://xcb.freedesktop.org/dist
$(package)_file_name=xcb-util-keysyms-$($(package)_version).tar.gz
$(package)_sha256_hash=1fa21c0cea3060caee7612b6577c1730da470b88cbdf846fa4e3e0ff78948e54
$(package)_dependencies=libxcb xproto

define $(package)_set_vars
$(package)_config_opts=--disable-shared --disable-devel-docs --without-doxygen
$(package)_config_opts += --disable-dependency-tracking --enable-option-checking
endef

define $(package)_preprocess_cmds
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub .
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
  rm -rf share/man share/doc lib/*.la
endef
