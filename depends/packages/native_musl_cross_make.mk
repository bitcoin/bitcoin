package=native_musl_cross_make
$(package)_version=fe915821b652a7fa37b34a596f47d8e20bc72338
$(package)_download_path=https://github.com/richfelker/musl-cross-make/archive
$(package)_download_file=$($(package)_version).tar.gz
$(package)_file_name=musl-cross-make-$($(package)_version).tar.gz
$(package)_sha256_hash=c5df9afd5efd41c97fc7f3866664ef0c91af0ff65116e27cd9cba078c7ab33ae
$(package)_patches=binutils_2_37.patch config.mak

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/binutils_2_37.patch && \
  cp -f $($(package)_patch_dir)/config.mak config.mak && \
  sed -i.old "s/TARGET =/TARGET = $(host)/g" config.mak
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) install && \
  mkdir -p "$($(package)_staging_prefix_dir)"/ && \
  rm -rf output/share && \
  cp -rf output/* "$($(package)_staging_prefix_dir)"/
endef
