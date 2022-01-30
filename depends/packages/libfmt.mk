package=libfmt
$(package)_version=7.1.3
$(package)_download_path=https://github.com/fmtlib/fmt/archive/
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=5cae7072042b3043e12d53d50ef404bbb76949dad1de368d7f993a15c8c05ecc

define $(package)_config_cmds
  $($(package)_cmake) -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=true
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
