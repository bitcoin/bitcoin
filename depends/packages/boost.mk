package=boost
$(package)_version=1.81.0
$(package)_download_path=https://archives.boost.io/release/$($(package)_version)/source/
$(package)_file_name=boost_$(subst .,_,$($(package)_version)).tar.gz
$(package)_sha256_hash=205666dea9f6a7cfed87c7a6dfbeb52a2c1b9de55712c9c1a87735d7181452b6

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/include && \
  cp -r boost $($(package)_staging_prefix_dir)/include
endef
