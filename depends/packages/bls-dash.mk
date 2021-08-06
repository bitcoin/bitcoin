package=bls-dash
$(package)_version=1.1.2
$(package)_download_path=https://github.com/syscoin/bls-signatures/archive
$(package)_download_file=$($(package)_version).tar.gz
$(package)_file_name=$(package)-$($(package)_download_file)
$(package)_build_subdir=build
$(package)_sha256_hash=83c7dd09f27cf9d63f4f913df6598772be50c0f4696649ca6d702a64a996d806
$(package)_dependencies=gmp cmake

$(package)_relic_version=3dd26f5ae28234e02fa4da375422ba954b7b1e53
$(package)_relic_download_path=https://github.com/relic-toolkit/relic/archive
$(package)_relic_download_file=$($(package)_relic_version).tar.gz
$(package)_relic_file_name=relic-toolkit-$($(package)_relic_download_file)
$(package)_relic_build_subdir=relic
$(package)_relic_sha256_hash=701d65ab08228c6db39f08c8b2aa4f5e4d6da3bd024eb88f1b4b1ee33e07e655

$(package)_extra_sources=$($(package)_relic_file_name)

define $(package)_fetch_cmds
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_download_file),$($(package)_file_name),$($(package)_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_relic_download_path),$($(package)_relic_download_file),$($(package)_relic_file_name),$($(package)_relic_sha256_hash))
endef

define $(package)_extract_cmds
  mkdir -p $($(package)_extract_dir) && \
  echo "$($(package)_sha256_hash)  $($(package)_source)" > $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_relic_sha256_hash)  $($(package)_source_dir)/$($(package)_relic_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  $(build_SHA256SUM) -c $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  tar --strip-components=1 -xf $($(package)_source) -C . && \
  cp $($(package)_source_dir)/$($(package)_relic_file_name) .
endef

define $(package)_set_vars
  $(package)_config_opts=-DCMAKE_INSTALL_PREFIX=$($(package)_staging_dir)/$(host_prefix)
  $(package)_config_opts+= -DCMAKE_PREFIX_PATH=$(host_prefix)
  $(package)_config_opts+= -DSTLIB=ON -DSHLIB=OFF -DSTBIN=ON
  $(package)_config_opts+= -DBUILD_BLS_PYTHON_BINDINGS=0 -DBUILD_BLS_TESTS=0 -DBUILD_BLS_BENCHMARKS=0 -DCMAKE_BUILD_TYPE=Release
  $(package)_config_opts_linux=-DOPSYS=LINUX -DCMAKE_SYSTEM_NAME=Linux
  $(package)_config_opts_darwin=-DOPSYS=MACOSX -DCMAKE_SYSTEM_NAME=Darwin
  $(package)_config_opts_mingw32=-DOPSYS=WINDOWS -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_SHARED_LIBRARY_LINK_C_FLAGS=""
  $(package)_config_opts_i686+= -DWSIZE=32
  $(package)_config_opts_x86_64+= -DWSIZE=64
  $(package)_config_opts_arm+= -DWSIZE=32
  $(package)_config_opts_armv7l+= -DWSIZE=32

  ifneq ($(darwin_native_toolchain),)
    $(package)_config_opts_darwin+= -DCMAKE_AR="$($(package)_ar)"
    $(package)_config_opts_darwin+= -DCMAKE_RANLIB="$($(package)_ranlib)"
  endif
  $(package)_cppflags+=-UBLSALLOC_SODIUM
endef

define $(package)_preprocess_cmds
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub . && \
  sed -i.old "s|GIT_REPOSITORY https://github.com/relic-toolkit/relic.git|URL \"../../relic-toolkit-$($(package)_relic_version).tar.gz\"|" src/CMakeLists.txt && \
  sed -i.old "s|GIT_TAG        .*RELIC_GIT_TAG.*|URL_HASH SHA256=$($(package)_relic_sha256_hash)|" src/CMakeLists.txt
endef

define $(package)_config_cmds
  export CC="$($(package)_cc)" && \
  export CXX="$($(package)_cxx)" && \
  export CFLAGS="$($(package)_cflags) $($(package)_cppflags)" && \
  export CXXFLAGS="$($(package)_cxxflags) $($(package)_cppflags) -std=c++11" && \
  export LDFLAGS="$($(package)_ldflags)" && \
  $(host_prefix)/bin/cmake ../ $($(package)_config_opts)
endef

define $(package)_build_cmds
  $(MAKE) $($(package)_build_opts)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef