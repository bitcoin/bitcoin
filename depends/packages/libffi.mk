package := libffi
$(package)_version := 3.4.2
$(package)_download_path := https://github.com/libffi/$(package)/releases/download/v$($(package)_version)
$(package)_file_name := $(package)-$($(package)_version).tar.gz
$(package)_sha256_hash := 540fb721619a6aba3bdeef7d940d8e9e0e6d2c193595bc243241b77ff9e93620

define $(package)_set_vars
  $(package)_config_opts := --enable-option-checking --disable-dependency-tracking
  $(package)_config_opts += --enable-shared --disable-static --disable-docs
  $(package)_config_opts += --disable-multi-os-directory
endef

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
