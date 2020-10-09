package=chia_bls
$(package)_version=v20201012
# It's actually from https://github.com/Chia-Network/bls-signatures, but we have so many patches atm that it's forked
$(package)_download_path=https://github.com/syscoin/bls-signatures/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=90f869f0dfd862e453b6ec856ea4904cba4cbbd02ba924a61c0f6ff87a2e72de
$(package)_dependencies=gmp
#$(package)_patches=...TODO (when we switch back to https://github.com/Chia-Network/bls-signatures)

#define $(package)_preprocess_cmds
#  for i in $($(package)_patches); do patch -N -p1 < $($(package)_patch_dir)/$$$$i; done
#endef

define $(package)_set_vars
$(package)_config_opts= CC="$($(package)_cc)"
$(package)_config_opts+=CFLAGS="$($(package)_cflags) $($(package)_cppflags)"
$(package)_config_opts+=CXXFLAGS="$($(package)_cxxflags) $($(package)_cppflags)"
$(package)_config_opts+=RANLIB="$($(package)_ranlib)"
$(package)_config_opts+=AR="$($(package)_ar)"
$(package)_config_opts_darwin+=AR="$($(package)_libtool)"
$(package)_config_opts_darwin+=ARFLAGS="-o"
endef

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