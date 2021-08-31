package=native_b2
$(package)_version=$(boost_version)
$(package)_download_path=$(boost_download_path)
$(package)_file_name=$(boost_file_name)
$(package)_sha256_hash=$(boost_sha256_hash)
$(package)_build_subdir=tools/build/src/engine
ifneq (,$(findstring clang,$($(package)_cxx)))
$(package)_toolset_$(host_os)=clang
else
$(package)_toolset_$(host_os)=gcc
endif

define $(package)_build_cmds
  CXX="$($(package)_cxx)" CXXFLAGS="$($(package)_cxxflags)" ./build.sh "$($(package)_toolset_$(host_os))"
endef

define $(package)_stage_cmds
  mkdir -p "$($(package)_staging_prefix_dir)"/bin/ && \
  cp b2 "$($(package)_staging_prefix_dir)"/bin/
endef
