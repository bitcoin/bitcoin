package=native_cctools
$(package)_version=20af0932d47de9ee38ba421a532eb666787a16b2
$(package)_download_path=https://github.com/tpoechtrager/cctools-port/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=8a4fb31c873261cad4392300bfc440b798bbe4f90b83078bf08aa17d0dd8853e
$(package)_build_subdir=cctools

$(package)_clang_version=6.0.1
$(package)_clang_download_path=https://releases.llvm.org/$($(package)_clang_version)
$(package)_clang_download_file=clang+llvm-$($(package)_clang_version)-x86_64-linux-gnu-ubuntu-16.04.tar.xz
$(package)_clang_file_name=clang-llvm-$($(package)_clang_version)-x86_64-linux-gnu-ubuntu-16.04.tar.xz
$(package)_clang_sha256_hash=7ea204ecd78c39154d72dfc0d4a79f7cce1b2264da2551bb2eef10e266d54d91

$(package)_libtapi_version=3efb201881e7a76a21e0554906cf306432539cef
$(package)_libtapi_download_path=https://github.com/tpoechtrager/apple-libtapi/archive
$(package)_libtapi_download_file=$($(package)_libtapi_version).tar.gz
$(package)_libtapi_file_name=$($(package)_libtapi_version).tar.gz
$(package)_libtapi_sha256_hash=380c1ca37cfa04a8699d0887a8d3ee1ad27f3d08baba78887c73b09485c0fbd3

$(package)_extra_sources=$($(package)_clang_file_name)
$(package)_extra_sources += $($(package)_libtapi_file_name)

define $(package)_fetch_cmds
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_download_file),$($(package)_file_name),$($(package)_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_clang_download_path),$($(package)_clang_download_file),$($(package)_clang_file_name),$($(package)_clang_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_libtapi_download_path),$($(package)_libtapi_download_file),$($(package)_libtapi_file_name),$($(package)_libtapi_sha256_hash))
endef

define $(package)_extract_cmds
  mkdir -p $($(package)_extract_dir) && \
  echo "$($(package)_sha256_hash)  $($(package)_source)" > $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_clang_sha256_hash)  $($(package)_source_dir)/$($(package)_clang_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_libtapi_sha256_hash)  $($(package)_source_dir)/$($(package)_libtapi_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  $(build_SHA256SUM) -c $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  mkdir -p libtapi && \
  tar --no-same-owner --strip-components=1 -C libtapi -xf $($(package)_source_dir)/$($(package)_libtapi_file_name) && \
  mkdir -p toolchain/bin toolchain/lib/clang/6.0.1/include && \
  tar --no-same-owner --strip-components=1 -C toolchain -xf $($(package)_source_dir)/$($(package)_clang_file_name) && \
  tar --no-same-owner --strip-components=1 -xf $($(package)_source)
endef

define $(package)_set_vars
  $(package)_config_opts=--target=$(host) --disable-lto-support --with-libtapi=$($(package)_extract_dir)
  $(package)_ldflags+=-Wl,-rpath=\\$$$$$$$$\$$$$$$$$ORIGIN/../lib
  $(package)_cc=$($(package)_extract_dir)/toolchain/bin/clang
  $(package)_cxx=$($(package)_extract_dir)/toolchain/bin/clang++
endef

define $(package)_preprocess_cmds
  CC=$($(package)_cc) CXX=$($(package)_cxx) INSTALLPREFIX=$($(package)_extract_dir) ./libtapi/build.sh && \
  CC=$($(package)_cc) CXX=$($(package)_cxx) INSTALLPREFIX=$($(package)_extract_dir) ./libtapi/install.sh && \
  sed -i.old "/define HAVE_PTHREADS/d" $($(package)_build_subdir)/ld64/src/ld/InputFiles.h
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install && \
  mkdir -p $($(package)_staging_prefix_dir)/lib/ && \
  cd $($(package)_extract_dir) && \
  cp lib/libtapi.so.6 $($(package)_staging_prefix_dir)/lib/ && \
  cd $($(package)_extract_dir)/toolchain && \
  mkdir -p $($(package)_staging_prefix_dir)/lib/clang/$($(package)_clang_version)/include && \
  mkdir -p $($(package)_staging_prefix_dir)/bin $($(package)_staging_prefix_dir)/include && \
  cp bin/clang $($(package)_staging_prefix_dir)/bin/ &&\
  cp -P bin/clang++ $($(package)_staging_prefix_dir)/bin/ &&\
  cp lib/libLTO.so $($(package)_staging_prefix_dir)/lib/ && \
  cp -rf lib/clang/$($(package)_clang_version)/include/* $($(package)_staging_prefix_dir)/lib/clang/$($(package)_clang_version)/include/ && \
  cp bin/llvm-dsymutil $($(package)_staging_prefix_dir)/bin/$(host)-dsymutil && \
  if `test -d include/c++/`; then cp -rf include/c++/ $($(package)_staging_prefix_dir)/include/; fi && \
  if `test -d lib/c++/`; then cp -rf lib/c++/ $($(package)_staging_prefix_dir)/lib/; fi
endef
