package=native_comparisontool
$(package)_version=8c6666f
$(package)_download_path=https://github.com/theuni/bitcoind-comparisontool/raw/master
$(package)_file_name=pull-tests-$($(package)_version).jar
$(package)_sha256_hash=a865332b3827abcde684ab79f5f43c083b0b6a4c97ff5508c79f29fee24f11cd
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
