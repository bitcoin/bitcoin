package=native_comparisontool
$(package)_version=adfd3de7
$(package)_download_path=https://github.com/TheBlueMatt/test-scripts/raw/10222bfdace65a0c5f3bd4a766eeb6b3a8b869fb/
$(package)_file_name=pull-tests-$($(package)_version).jar
$(package)_sha256_hash=fd2282b112e35f339dbe3729b08a04834ad719f8c9c10eeec1178465e6d36a18
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
