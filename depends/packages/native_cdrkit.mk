package=native_cdrkit
$(package)_version=1.1.11
$(package)_download_path=https://distro.ibiblio.org/fatdog/source/600/c
$(package)_file_name=cdrkit-$($(package)_version).tar.bz2
$(package)_sha256_hash=b50d64c214a65b1a79afe3a964c691931a4233e2ba605d793eb85d0ac3652564
$(package)_patches=cdrkit-deterministic.patch

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/cdrkit-deterministic.patch
endef

# Starting with 10.1, GCC defaults to -fno-common, resulting in linking errors.
# Pass -fcommon to retain the legacy behaviour.
define $(package)_config_cmds
  $($(package)_cmake) -DCMAKE_C_FLAGS="$$($(1)_cflags) -fcommon"
endef

define $(package)_build_cmds
  $(MAKE) genisoimage
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) -C genisoimage install
endef

define $(package)_postprocess_cmds
  rm bin/isovfy bin/isoinfo bin/isodump bin/isodebug bin/devdump
endef
