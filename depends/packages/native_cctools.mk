package=native_cctools
$(package)_version=2ef2e931cf641547eb8a68cfebde61003587c9fd
$(package)_download_path=https://github.com/tpoechtrager/cctools-port/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=6b73269efdf5c58a070e7357b66ee760501388549d6a12b423723f45888b074b
$(package)_build_subdir=cctools
$(package)_dependencies=native_libtapi

define $(package)_set_vars
  $(package)_config_opts=--target=$(host) --enable-lto-support
  $(package)_config_opts+=--with-llvm-config=$(llvm_config_prog)
  $(package)_ldflags+=-Wl,-rpath=\\$$$$$$$$\$$$$$$$$ORIGIN/../lib
  $(package)_cc=$(clang_prog)
  $(package)_cxx=$(clangxx_prog)
endef

ifneq ($(strip $(FORCE_USE_SYSTEM_CLANG)),)
define $(package)_preprocess_cmds
  mkdir -p $($(package)_staging_prefix_dir)/lib && \
  cp $(llvm_lib_dir)/libLTO.so $($(package)_staging_prefix_dir)/lib/ && \
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub cctools
endef
else
define $(package)_preprocess_cmds
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub cctools
endef
endif

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm -rf share
endef
