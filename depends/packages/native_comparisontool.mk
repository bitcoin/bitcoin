package=native_comparisontool
$(package)_version=5caed78
$(package)_download_path=https://github.com/TheBlueMatt/test-scripts/raw/2d76ce92d68e6746988adde731318605a70e252c
$(package)_file_name=pull-tests-$($(package)_version).jar
$(package)_sha256_hash=b55a98828b17060e327c5dabe5e4631898f422c0cba07c46170930a9eaf5e7c0
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
