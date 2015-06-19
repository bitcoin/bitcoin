package=native_comparisontool
$(package)_version=be0eef7
$(package)_download_path=https://github.com/theuni/bitcoind-comparisontool/raw/master
$(package)_file_name=pull-tests-$($(package)_version).jar
$(package)_sha256_hash=46c2efa25e717751e1ef380d428761b1e37b43d6d1e92ac761b15ee92703e560
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
