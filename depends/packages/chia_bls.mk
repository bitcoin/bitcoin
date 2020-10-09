package=chia_bls
$(package)_version=v20201011
# It's actually from https://github.com/Chia-Network/bls-signatures, but we have so many patches atm that it's forked
$(package)_download_path=https://github.com/syscoin/bls-signatures/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=d08cc02cc63b174b51e4cd822598bfd9823ec7c07c03452cc1a8b0c8c0401b9e
$(package)_dependencies=gmp
#$(package)_patches=...TODO (when we switch back to https://github.com/Chia-Network/bls-signatures)

#define $(package)_preprocess_cmds
#  for i in $($(package)_patches); do patch -N -p1 < $($(package)_patch_dir)/$$$$i; done
#endef

define $(package)_config_cmds
  mkdir -p build && cd build && \
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  cd build && \
  $(MAKE) $($(package)_build_opts)
endef

define $(package)_stage_cmds
  cd build && \
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef