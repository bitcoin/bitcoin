package=libfmt
$(package)_version=7.1.3
$(package)_download_path=https://github.com/fmtlib/fmt/archive/
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=5cae7072042b3043e12d53d50ef404bbb76949dad1de368d7f993a15c8c05ecc

define $(package)_set_vars
$(package)_config_opts_x86_64_mingw32=-DCMAKE_TOOLCHAIN_FILE=$(BASEDIR)/cmake/mingw-w64-x86_64.cmake
endef

define $(package)_config_cmds
  cmake -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=true $($(package)_config_opts) .
endef

define $(package)_build_cmds
  $(MAKE) && \
  mkdir -p $($(package)_staging_dir)$(host_prefix)/include && \
  cp -a include/* $($(package)_staging_dir)$(host_prefix)/include/ && \
  mkdir -p $($(package)_staging_dir)$(host_prefix)/lib && \
  cp -a libfmt.a $($(package)_staging_dir)$(host_prefix)/lib/
endef