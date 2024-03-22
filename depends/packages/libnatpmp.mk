package=libnatpmp
$(package)_version=f2433bec24ca3d3f22a8a7840728a3ac177f94ba
$(package)_download_path=https://github.com/miniupnp/libnatpmp/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=ef84979950dfb3556705b63c9cd6c95501b75e887fba466234b187f3c9029669
$(package)_patches=no_libtool.patch

define $(package)_set_vars
  $(package)_build_opts=CC="$($(package)_cc)"
  $(package)_build_opts_mingw32=CPPFLAGS=-DNATPMP_STATICLIB
  $(package)_build_env+=CFLAGS="$($(package)_cflags) $($(package)_cppflags)" AR="$($(package)_ar)"
endef

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/no_libtool.patch
endef

define $(package)_build_cmds
  $(MAKE) libnatpmp.a $($(package)_build_opts)
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/include $($(package)_staging_prefix_dir)/lib &&\
  install *.h $($(package)_staging_prefix_dir)/include &&\
  install libnatpmp.a $($(package)_staging_prefix_dir)/lib
endef
