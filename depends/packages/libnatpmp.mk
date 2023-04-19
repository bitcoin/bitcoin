package=libnatpmp
$(package)_version=f2433bec24ca3d3f22a8a7840728a3ac177f94ba
$(package)_download_path=https://github.com/miniupnp/libnatpmp/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=ef84979950dfb3556705b63c9cd6c95501b75e887fba466234b187f3c9029669
$(package)_build_subdir=build

define $(package)_config_cmds
  $($(package)_cmake) -S .. -B .
endef

define $(package)_build_cmds
  $(MAKE) natpmp
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/include $($(package)_staging_prefix_dir)/lib && \
  install ../natpmp.h  ../natpmp_declspec.h $($(package)_staging_prefix_dir)/include && \
  install libnatpmp.a $($(package)_staging_prefix_dir)/lib
endef
