# filepath: /Users/owner/Documents/bitcoin-build/bitcoin-src/depends/packages/openapi-generator.mk
package=openapi-generator-cli
$(package)_version=7.12.0
$(package)_download_path=https://repo1.maven.org/maven2/org/openapitools/openapi-generator-cli/$($(package)_version)/
$(package)_file_name=openapi-generator-cli-$($(package)_version).jar
$(package)_sha256_hash=33e7dfa7a1f04d58405ee12ae19e2c6fc2a91497cf2e56fa68f1875a95cbf220

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/bin && \
  cp openapi-generator-cli-$($(package)_version).jar $($(package)_staging_prefix_dir)/bin/
endef
