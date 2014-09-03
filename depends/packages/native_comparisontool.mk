package=native_comparisontool
$(package)_version=bca2f2abac3
$(package)_download_path=https://github.com/theuni/bitcoind-comparisontool/raw/master/
$(package)_file_name=pull-tests-$($(package)_version).jar
$(package)_sha256_hash=cef592a839c3050d8952a8a16bee3d565d82278e90af0d88c2f0f6f2ec15d4df
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
  mv $(SOURCES_PATH)/$($(package)_file_name) $($(package)_staging_prefix_dir)/share/$($(package)_install_dirname)/$($(package)_install_filename)
endef
