package=libnatpmp
$(package)_version=20150609
$(package)_download_path=https://miniupnp.tuxfamily.org/files/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=e1aa9c4c4219bc06943d6b2130f664daee213fb262fcb94dd355815b8f4536b0

define $(package)_set_vars
  $(package)_build_opts=CC="$($(package)_cc)"
endef

define $(package)_build_cmds
  $(MAKE) libnatpmp.a $($(package)_build_opts)
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/include $($(package)_staging_prefix_dir)/lib &&\
  install *.h $($(package)_staging_prefix_dir)/include &&\
  install libnatpmp.a $($(package)_staging_prefix_dir)/lib
endef
