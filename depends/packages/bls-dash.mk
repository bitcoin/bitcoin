package=bls-dash
$(package)_version=1.2.0
$(package)_download_path=https://github.com/dashpay/bls-signatures/archive
$(package)_download_file=$($(package)_version).tar.gz
$(package)_file_name=$(package)-$($(package)_download_file)
$(package)_build_subdir=build
$(package)_sha256_hash=94e49f3eaa29bc1f354cd569c00f4f4314d1c8ab4758527c248b67da9686135a
$(package)_dependencies=gmp cmake
$(package)_darwin_triplet=x86_64-apple-darwin19

$(package)_relic_version=aecdcae7956f542fbee2392c1f0feb0a8ac41dc5
$(package)_relic_download_path=https://github.com/relic-toolkit/relic/archive
$(package)_relic_download_file=$($(package)_relic_version).tar.gz
$(package)_relic_file_name=relic-$($(package)_relic_download_file)
$(package)_relic_build_subdir=relic
$(package)_relic_sha256_hash=f2de6ebdc9def7077f56c83c8b06f4da5bacc36b709514bd550a92a149e9fa1d

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
  $(package)_config_opts=-DSTLIB=ON -DSHLIB=OFF -DSTBIN=OFF
  $(package)_config_opts+= -DBUILD_BLS_PYTHON_BINDINGS=0 -DBUILD_BLS_TESTS=0 -DBUILD_BLS_BENCHMARKS=0 -DCMAKE_CXX_STANDARD_REQUIRED=ON -DCMAKE_C_STANDARD=99
  $(package)_config_opts_linux=-DOPSYS=LINUX -DCMAKE_SYSTEM_NAME=Linux -DMULTI=PTHREAD
  $(package)_config_opts_darwin=-DOPSYS=MACOSX -DCMAKE_SYSTEM_NAME=Darwin -DMULTI=PTHREAD
  $(package)_config_opts_mingw32=-DOPSYS=WINDOWS -DCMAKE_SYSTEM_NAME=Windows -DMULTI=PTHREAD -DCMAKE_SHARED_LIBRARY_LINK_C_FLAGS=""
  $(package)_config_opts_i686+= -DWSIZE=32
  $(package)_config_opts_x86_64+= -DWSIZE=64
  $(package)_config_opts_arm+= -DWSIZE=32
  $(package)_config_opts_armv7l+= -DWSIZE=32

  ifneq ($(darwin_native_toolchain),)
    $(package)_config_opts_darwin+= -DCMAKE_AR="$($(package)_ar)"
    $(package)_config_opts_darwin+= -DCMAKE_RANLIB="$($(package)_ranlib)"
  endif
  $(package)_cflags_linux+= -std=c99 -funroll-loops -fomit-frame-pointer
  $(package)_cflags_darwin+= -std=c99 -funroll-loops -fomit-frame-pointer
  $(package)_cflags_arm+= -std=c99 -funroll-loops -fomit-frame-pointer
  $(package)_cflags_armv7l+= -std=c99 -funroll-loops -fomit-frame-pointer
  $(package)_cppflags+= -UBLSALLOC_SODIUM
  $(package)_cxxflags_linux=-fPIC
  $(package)_cxxflags_android=-fPIC
endef

define $(package)_preprocess_cmds
  sed -i.old "s|GIT_REPOSITORY https://github.com/Chia-Network/relic.git|URL \"../../relic-$($(package)_relic_version).tar.gz\"|" CMakeLists.txt && \
  sed -i.old "s|RELIC_GIT_TAG \".*\"|RELIC_GIT_TAG \"\"|" CMakeLists.txt
endef

define $(package)_config_cmds
  $($(package)_cmake) ../ $($(package)_config_opts)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef