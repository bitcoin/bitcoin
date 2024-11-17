package=miniupnpc
$(package)_version=2.2.2
$(package)_download_path=https://miniupnp.tuxfamily.org/files/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=888fb0976ba61518276fe1eda988589c700a3f2a69d71089260d75562afd3687
$(package)_patches=dont_leak_info.patch respect_mingw_cflags.patch no_libtool.patch

# Next time this package is updated, ensure that _WIN32_WINNT is still properly set.
# See discussion in https://github.com/bitcoin/bitcoin/pull/25964.
define $(package)_set_vars
$(package)_build_opts=CC="$($(package)_cc)"
$(package)_build_opts_mingw32=-f Makefile.mingw CFLAGS="$($(package)_cflags) -D_WIN32_WINNT=0x0601"
$(package)_build_env+=CFLAGS="$($(package)_cflags) $($(package)_cppflags)" AR="$($(package)_ar)"
endef

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/dont_leak_info.patch && \
  patch -p1 < $($(package)_patch_dir)/respect_mingw_cflags.patch && \
  patch -p1 < $($(package)_patch_dir)/no_libtool.patch
endef

define $(package)_build_cmds
	$(MAKE) libminiupnpc.a $($(package)_build_opts)
endef

define $(package)_stage_cmds
	mkdir -p $($(package)_staging_prefix_dir)/include/miniupnpc $($(package)_staging_prefix_dir)/lib &&\
	install *.h $($(package)_staging_prefix_dir)/include/miniupnpc &&\
	install libminiupnpc.a $($(package)_staging_prefix_dir)/lib
endef

define $(package)_postprocess_cmds
  rm -rf bin && \
  rm -rf share
endef
