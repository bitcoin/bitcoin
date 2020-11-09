package=libevent
$(package)_version=2.1.11-stable
$(package)_download_path=https://github.com/libevent/libevent/archive/
$(package)_file_name=release-$($(package)_version).tar.gz
$(package)_sha256_hash=229393ab2bf0dc94694f21836846b424f3532585bac3468738b7bf752c03901e
$(package)_patches=0001-fix-windows-getaddrinfo.patch

define $(package)_preprocess_cmds
   patch -p1 < $($(package)_patch_dir)/0001-fix-windows-getaddrinfo.patch && \
  ./autogen.sh
endef

# When building for Windows, we set _WIN32_WINNT to target the same Windows
# version as we do in configure. Due to quirks in libevents build system, this
# is also required to enable support for ipv6. See #19375.
define $(package)_set_vars
  $(package)_config_opts=--disable-shared --disable-openssl --disable-libevent-regress --disable-samples
  $(package)_config_opts += --disable-dependency-tracking --enable-option-checking
  $(package)_config_opts_release=--disable-debug-mode
  $(package)_config_opts_linux=--with-pic
  $(package)_config_opts_android=--with-pic
  $(package)_cppflags_mingw32=-D_WIN32_WINNT=0x0601
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm lib/*.la
endef
