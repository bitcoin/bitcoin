package=libnatpmp
$(package)_version=07004b97cf691774efebe70404cf22201e4d330d
$(package)_download_path=https://github.com/miniupnp/libnatpmp/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=9321953ceb39d07c25463e266e50d0ae7b64676bb3a986d932b18881ed94f1fb

define $(package)_set_vars
  $(package)_build_opts=CC="$($(package)_cc)"
  $(package)_build_opts_mingw32=CPPFLAGS=-DNATPMP_STATICLIB
  $(package)_build_opts_darwin=LIBTOOL="$($(package)_libtool)"
  $(package)_build_env+=CFLAGS="$($(package)_cflags) $($(package)_cppflags)" AR="$($(package)_ar)"
endef

define $(package)_build_cmds
  $(MAKE) libnatpmp.a $($(package)_build_opts)
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/include $($(package)_staging_prefix_dir)/lib &&\
  install *.h $($(package)_staging_prefix_dir)/include &&\
  install libnatpmp.a $($(package)_staging_prefix_dir)/lib
endef
