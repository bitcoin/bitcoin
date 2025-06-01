package=sqlite
$(package)_version=3460100
$(package)_download_path=https://sqlite.org/2024/
$(package)_file_name=sqlite-autoconf-$($(package)_version).tar.gz
$(package)_sha256_hash=67d3fe6d268e6eaddcae3727fce58fcc8e9c53869bdd07a0c61e38ddf2965071

define $(package)_set_vars
$(package)_config_opts=--disable-shared --disable-readline --disable-dynamic-extensions --enable-option-checking
$(package)_config_opts+= --disable-rtree --disable-fts4 --disable-fts5
# We avoid using `--enable-debug` because it overrides CFLAGS, a behavior we want to prevent.
$(package)_cppflags_debug += -DSQLITE_DEBUG
$(package)_cppflags+=-DSQLITE_DQS=0 -DSQLITE_DEFAULT_MEMSTATUS=0 -DSQLITE_OMIT_DEPRECATED
$(package)_cppflags+=-DSQLITE_OMIT_SHARED_CACHE -DSQLITE_OMIT_JSON -DSQLITE_LIKE_DOESNT_MATCH_BLOBS
$(package)_cppflags+=-DSQLITE_OMIT_DECLTYPE -DSQLITE_OMIT_PROGRESS_CALLBACK -DSQLITE_OMIT_AUTOINIT
endef

define $(package)_preprocess_cmds
  cp -f $(BASEDIR)/config.guess $(BASEDIR)/config.sub .
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE) libsqlite3.la
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install-libLTLIBRARIES install-includeHEADERS
endef

define $(package)_postprocess_cmds
  rm lib/*.la
endef
