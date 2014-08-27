package=native_comparisontool
$(package)_version=1
$(package)_download_path=https://github.com/TheBlueMatt/test-scripts/raw/master/BitcoindComparisonTool_jar
$(package)_file_name=BitcoindComparisonTool.jar
$(package)_sha256_hash=a08b1a55523e7f57768cb66c35f47a926710e5b6c82822e1ccfbe38fcce37db2
$(package)_guava_file_name=guava-13.0.1.jar
$(package)_guava_sha256_hash=feb4b5b2e79a63b72ec47a693b1cf35cf1cea1f60a2bb2615bf21f74c7a60bb0
$(package)_h2_file_name=h2-1.3.167.jar
$(package)_h2_sha256_hash=fa97521a2e72174485a96276bcf6f573d5e44ca6aba2f62de87b33b5bb0d4b91
$(package)_sc-light-jdk15on_file_name=sc-light-jdk15on-1.47.0.2.jar
$(package)_sc-light-jdk15on_sha256_hash=931f39d351429fb96c2f749e7ecb1a256a8ebbf5edca7995c9cc085b94d1841d
$(package)_slf4j-api_file_name=slf4j-api-1.6.4.jar
$(package)_slf4j-api_sha256_hash=367b909030f714ee1176ab096b681e06348f03385e98d1bce0ed801b5452357e
$(package)_slf4j-jdk14_file_name=slf4j-jdk14-1.6.4.jar
$(package)_slf4j-jdk14_sha256_hash=064bd81796710f713f9f4a2309c0e032309934c2d2b4f7d3b6958325e584e13f

define $(package)_fetch_cmds
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_file_name),$($(package)_file_name),$($(package)_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_guava_file_name),$($(package)_guava_file_name),$($(package)_guava_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_h2_file_name),$($(package)_h2_file_name),$($(package)_h2_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_sc-light-jdk15on_file_name),$($(package)_sc-light-jdk15on_file_name),$($(package)_sc-light-jdk15on_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_slf4j-api_file_name),$($(package)_slf4j-api_file_name),$($(package)_slf4j-api_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_slf4j-jdk14_file_name),$($(package)_slf4j-jdk14_file_name),$($(package)_slf4j-jdk14_sha256_hash))
endef

define $(package)_extract_cmds
echo none
endef

define $(package)_configure_cmds
endef

define $(package)_build_cmds
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/share/BitcoindComparisonTool_jar && \
  cp $(SOURCES_PATH)/$($(package)_file_name) $($(package)_staging_prefix_dir)/share/BitcoindComparisonTool_jar/  && \
  cp $(SOURCES_PATH)/$($(package)_guava_file_name) $($(package)_staging_prefix_dir)/share/BitcoindComparisonTool_jar/  && \
  cp $(SOURCES_PATH)/$($(package)_h2_file_name) $($(package)_staging_prefix_dir)/share/BitcoindComparisonTool_jar/  && \
  cp $(SOURCES_PATH)/$($(package)_sc-light-jdk15on_file_name) $($(package)_staging_prefix_dir)/share/BitcoindComparisonTool_jar/  && \
  cp $(SOURCES_PATH)/$($(package)_slf4j-api_file_name) $($(package)_staging_prefix_dir)/share/BitcoindComparisonTool_jar/  && \
  cp $(SOURCES_PATH)/$($(package)_slf4j-jdk14_file_name) $($(package)_staging_prefix_dir)/share/BitcoindComparisonTool_jar/
endef
