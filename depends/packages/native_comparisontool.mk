package=native_comparisontool
$(package)_version=f56eec3
$(package)_download_path=https://github.com/TheBlueMatt/test-scripts/raw/89dbae28f04eae31cb05a4011d9b810440956b68
$(package)_file_name=pull-tests-$($(package)_version).jar
$(package)_sha256_hash=e3a5a07aa72f2706235a9ca4c68951b7983448db2f13059a6ef1da38245dae94
$(package)_install_dirname=BitcoindComparisonTool_jar
$(package)_install_filename=BitcoindComparisonTool.jar

define $(package)_extract_cmds
endef

define $(package)_configure_cmds
endef

define $(package)_build_cmds
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/share/$($(package)_install_dirname) && \
  cp $($(package)_source) $($(package)_staging_prefix_dir)/share/$($(package)_install_dirname)/$($(package)_install_filename)
endef
