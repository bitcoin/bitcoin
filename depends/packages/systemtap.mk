package=systemtap
$(package)_version=4.8
$(package)_download_path=https://sourceware.org/ftp/systemtap/releases/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=cbd50a4eba5b261394dc454c12448ddec73e55e6742fda7f508f9fbc1331c223
$(package)_patches=remove_SDT_ASM_SECTION_AUTOGROUP_SUPPORT_check.patch fix_variadic_warning.patch

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/remove_SDT_ASM_SECTION_AUTOGROUP_SUPPORT_check.patch && \
  patch -p1 < $($(package)_patch_dir)/fix_variadic_warning.patch && \
  mkdir -p $($(package)_staging_prefix_dir)/include/sys && \
  cp includes/sys/sdt.h $($(package)_staging_prefix_dir)/include/sys/sdt.h
endef
