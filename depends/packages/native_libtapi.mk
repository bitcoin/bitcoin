package=native_libtapi
$(package)_version=eb33a59f2e30ff9724dc1ea8bee8b5229b0557c9
$(package)_download_path=https://github.com/tpoechtrager/apple-libtapi/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=d4d46c64622f13d6938cecf989046d9561011bb59e8ee835f8f39825d67f578f
$(package)_patches=disable_zlib.patch

ifeq ($(strip $(FORCE_USE_SYSTEM_CLANG)),)
$(package)_dependencies=native_clang
endif

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/disable_zlib.patch
endef

define $(package)_build_cmds
  CC=$(clang_prog) CXX=$(clangxx_prog) INSTALLPREFIX=$($(package)_staging_prefix_dir) ./build.sh
endef

define $(package)_stage_cmds
  ./install.sh
endef
