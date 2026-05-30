package=sqlite
$(package)_version=3500400
$(package)_download_path=https://sqlite.org/2025/
$(package)_file_name=sqlite-autoconf-$($(package)_version).tar.gz
$(package)_sha256_hash=a3db587a1b92ee5ddac2f66b3edb41b26f9c867275782d46c3a088977d6a5b18
$(package)_patches = autosetup-fixup.patch

define $(package)_set_vars
$(package)_config_env := CC_FOR_BUILD="$$(build_CC)"
$(package)_config_opts = --disable-shared --disable-readline --disable-rtree
$(package)_config_opts += --disable-fts4 --disable-fts5
$(package)_config_opts_debug += --debug
$(package)_cppflags += -DSQLITE_DQS=0 -DSQLITE_DEFAULT_MEMSTATUS=0 -DSQLITE_OMIT_DEPRECATED
$(package)_cppflags += -DSQLITE_OMIT_SHARED_CACHE -DSQLITE_OMIT_JSON -DSQLITE_LIKE_DOESNT_MATCH_BLOBS
$(package)_cppflags += -DSQLITE_OMIT_DECLTYPE -DSQLITE_OMIT_PROGRESS_CALLBACK -DSQLITE_OMIT_AUTOINIT
$(package)_cppflags += -DSQLITE_OMIT_LOAD_EXTENSION
endef

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/autosetup-fixup.patch
endef

# Remove --with-pic, which is applied globally to configure
# invocations but is incompatible with Autosetup
define $(package)_config_cmds
  $$(filter-out --with-pic,$($(package)_autoconf))
endef

define $(package)_build_cmds
  $(MAKE) libsqlite3.a
endef

ifneq ($(MSYS_STAGING),)
# On native Windows (MSYS2), sqlite's autosetup canonicalizes --prefix to a
# Windows path (C:/...). sqlite's generated Makefile then lists the install
# directories ($(DESTDIR)$(libdir) etc.) as bare rule targets; the drive-letter
# colon makes GNU make read them as "multiple target patterns" and abort with
#   Makefile:87: *** multiple target patterns.  Stop.
# We only need the static lib and the public headers, so install them by hand
# into the canonical staging prefix and skip the broken `make install` rules.
define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/lib $($(package)_staging_prefix_dir)/include && \
  cp -f libsqlite3.a $($(package)_staging_prefix_dir)/lib/ && \
  cp -f sqlite3.h sqlite3ext.h $($(package)_staging_prefix_dir)/include/
endef
else
define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install-headers install-lib
endef
endif
