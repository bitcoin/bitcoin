package=miniupnpc
$(package)_version=2.2.5
$(package)_download_path=https://miniupnp.tuxfamily.org/files/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=38acd5f4602f7cf8bcdc1ec30b2d58db2e9912e5d9f5350dd99b06bfdffb517c
$(package)_patches=dont_leak_info.patch

define $(package)_set_vars
$(package)_build_opts=CC="$($(package)_cc)"
$(package)_build_opts_darwin=LIBTOOL="$($(package)_libtool)"
$(package)_build_opts_mingw32=-f Makefile.mingw CFLAGS="$($(package)_cflags) -D_WIN32_WINNT=0x0601"
$(package)_build_env+=CFLAGS="$($(package)_cflags) $($(package)_cppflags)" AR="$($(package)_ar)"
endef

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/dont_leak_info.patch
endef

define $(package)_build_cmds
	$(MAKE) build/libminiupnpc.a build/miniupnpc.pc $($(package)_build_opts)
endef

define $(package)_stage_cmds
	mkdir -p $($(package)_staging_prefix_dir)/include/miniupnpc $($(package)_staging_prefix_dir)/lib/pkgconfig && \
	install include/*.h $($(package)_staging_prefix_dir)/include/miniupnpc && \
	install build/miniupnpc.pc $($(package)_staging_prefix_dir)/lib/pkgconfig && \
	install build/libminiupnpc.a $($(package)_staging_prefix_dir)/lib
endef
