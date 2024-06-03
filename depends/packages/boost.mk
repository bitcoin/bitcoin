package=boost
$(package)_version=1.81.0
$(package)_download_path=https://archives.boost.io/release/$($(package)_version)/source/
$(package)_file_name=boost_$(subst .,_,$($(package)_version)).tar.bz2
$(package)_sha256_hash=71feeed900fbccca04a3b4f2f84a7c217186f28a940ed8b7ed4725986baf99fa
$(package)_patches=process_macos_sdk.patch

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/process_macos_sdk.patch
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/include && \
  cp -r boost $($(package)_staging_prefix_dir)/include
endef
