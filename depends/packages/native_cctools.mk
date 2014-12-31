package=native_cctools
$(package)_version=59d21d2c793c51d205c8b4ab14b9b28e63c72445
$(package)_download_path=https://github.com/theuni/cctools-port/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=e13c7129b9d496adf4d674156b4a1d72d8b936f29f377aae8f8b5e7b650cc305
$(package)_build_subdir=cctools
$(package)_dependencies=native_libuuid native_openssl
$(package)_clang_version=3.3
$(package)_clang_download_path=http://llvm.org/releases/$($(package)_clang_version)
$(package)_clang_download_file=clang+llvm-$($(package)_clang_version)-amd64-Ubuntu-12.04.2.tar.gz
$(package)_clang_file_name=clang-llvm-$($(package)_clang_version)-amd64-Ubuntu-12.04.2.tar.gz
$(package)_clang_sha256_hash=60d8f69f032d62ef61bf527857ebb933741ec3352d4d328c5516aa520662dab7
define $(package)_fetch_cmds
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_download_file),$($(package)_file_name),$($(package)_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_clang_download_path),$($(package)_clang_download_file),$($(package)_clang_file_name),$($(package)_clang_sha256_hash))
endef

define $(package)_extract_cmds
  mkdir -p toolchain/bin toolchain/lib/clang/3.5/include && \
  tar --strip-components=1 -C toolchain -xf $($(package)_source_dir)/$($(package)_clang_file_name) && \
  echo "#!/bin/sh" > toolchain/bin/$(host)-dsymutil && \
  echo "exit 0" >> toolchain/bin/$(host)-dsymutil && \
  chmod +x toolchain/bin/$(host)-dsymutil && \
  tar --strip-components=1 -xf $($(package)_source)
endef

define $(package)_set_vars
$(package)_config_opts=--target=$(host) CC=toolchain/clang CXX=toolchain/clang++
$(package)_ldflags+=-Wl,-rpath=\\$$$$$$$$\$$$$$$$$ORIGIN/../lib
endef

define $(package)_preprocess_cmds
  cd $($(package)_build_subdir); ./autogen.sh
endef

define $(package)_config_cmds
  export "PATH=$($(package)_extract_dir)/toolchain/bin:$(PATH)" && \
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  export "PATH=$($(package)_extract_dir)/toolchain/bin:$(PATH)" && \
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install && \
  cd $($(package)_extract_dir)/toolchain && \
  mkdir -p $($(package)_staging_prefix_dir)/lib/clang/$($(package)_clang_version)/include && \
  mkdir -p $($(package)_staging_prefix_dir)/bin $($(package)_staging_prefix_dir)/include && \
  cp -P bin/clang bin/clang++ $($(package)_staging_prefix_dir)/bin/ &&\
  cp lib/libLTO.so $($(package)_staging_prefix_dir)/lib/ && \
  cp -rf lib/clang/$($(package)_clang_version)/include/* $($(package)_staging_prefix_dir)/lib/clang/$($(package)_clang_version)/include/ && \
  cp bin/$(host)-dsymutil $($(package)_staging_prefix_dir)/bin && \
  if `test -d include/c++/`; then cp -rf include/c++/ $($(package)_staging_prefix_dir)/include/; fi && \
  if `test -d lib/c++/`; then cp -rf lib/c++/ $($(package)_staging_prefix_dir)/lib/; fi
endef
