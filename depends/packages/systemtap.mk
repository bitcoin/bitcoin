package=systemtap
$(package)_version=4.7
$(package)_download_path=https://sourceware.org/systemtap/ftp/releases/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=43a0a3db91aa4d41e28015b39a65e62059551f3cc7377ebf3a3a5ca7339e7b1f
$(package)_patches=remove_SDT_ASM_SECTION_AUTOGROUP_SUPPORT_check.patch

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/remove_SDT_ASM_SECTION_AUTOGROUP_SUPPORT_check.patch && \
  mkdir -p $($(package)_staging_prefix_dir)/include/sys && \
  cp includes/sys/sdt.h $($(package)_staging_prefix_dir)/include/sys/sdt.h
endef
