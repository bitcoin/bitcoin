package=systemtap
$(package)_version=5.3
$(package)_download_path=https://sourceware.org/ftp/systemtap/releases/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=966a360fb73a4b65a8d0b51b389577b3c4f92a327e84aae58682103e8c65a69a
$(package)_patches=remove_SDT_ASM_SECTION_AUTOGROUP_SUPPORT_check.patch

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/remove_SDT_ASM_SECTION_AUTOGROUP_SUPPORT_check.patch && \
  mkdir -p $($(package)_staging_prefix_dir)/include/sys && \
  cp includes/sys/sdt.h $($(package)_staging_prefix_dir)/include/sys/sdt.h
endef
