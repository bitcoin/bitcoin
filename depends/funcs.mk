define int_vars
#Set defaults for vars which may be overridden per-package
$(1)_cc=$$($$($(1)_type)_CC)
$(1)_cxx=$$($$($(1)_type)_CXX)
$(1)_objc=$$($$($(1)_type)_OBJC)
$(1)_objcxx=$$($$($(1)_type)_OBJCXX)
$(1)_ar=$$($$($(1)_type)_AR)
$(1)_ranlib=$$($$($(1)_type)_RANLIB)
$(1)_libtool=$$($$($(1)_type)_LIBTOOL)
$(1)_nm=$$($$($(1)_type)_NM)
$(1)_cflags=$$($$($(1)_type)_CFLAGS) \
            $$($$($(1)_type)_$$(release_type)_CFLAGS)
$(1)_cxxflags=$$($$($(1)_type)_CXXFLAGS) \
              $$($$($(1)_type)_$$(release_type)_CXXFLAGS)
$(1)_ldflags=$$($$($(1)_type)_LDFLAGS) \
             $$($$($(1)_type)_$$(release_type)_LDFLAGS) \
             -L$$($($(1)_type)_prefix)/lib
$(1)_cppflags=$$($$($(1)_type)_CPPFLAGS) \
              $$($$($(1)_type)_$$(release_type)_CPPFLAGS) \
              -I$$($$($(1)_type)_prefix)/include
$(1)_recipe_hash:=
endef

define int_get_all_dependencies
$(sort $(foreach dep,$(2),$(2) $(call int_get_all_dependencies,$(1),$($(dep)_dependencies))))
endef

define fetch_file_inner
    ( mkdir -p $$($(1)_download_dir) && echo Fetching $(3) from $(2) && \
    $(build_DOWNLOAD) "$$($(1)_download_dir)/$(4).temp" "$(2)/$(3)" && \
    echo "$(5)  $$($(1)_download_dir)/$(4).temp" > $$($(1)_download_dir)/.$(4).hash && \
    $(build_SHA256SUM) -c $$($(1)_download_dir)/.$(4).hash && \
    mv $$($(1)_download_dir)/$(4).temp $$($(1)_source_dir)/$(4) && \
    rm -rf $$($(1)_download_dir) )
endef

define fetch_file
    ( test -f $$($(1)_source_dir)/$(4) || \
    ( $(call fetch_file_inner,$(1),$(2),$(3),$(4),$(5)) || \
      $(call fetch_file_inner,$(1),$(FALLBACK_DOWNLOAD_PATH),$(3),$(4),$(5))))
endef

define int_get_build_recipe_hash
$(eval $(1)_all_file_checksums:=$(shell $(build_SHA256SUM) $(meta_depends) packages/$(1).mk $(addprefix $(PATCHES_PATH)/$(1)/,$($(1)_patches)) | cut -d" " -f1))
$(eval $(1)_recipe_hash:=$(shell echo -n "$($(1)_all_file_checksums)" | $(build_SHA256SUM) | cut -d" " -f1))
endef

define int_get_build_id
$(eval $(1)_dependencies += $($(1)_$(host_arch)_$(host_os)_dependencies) $($(1)_$(host_os)_dependencies))
$(eval $(1)_all_dependencies:=$(call int_get_all_dependencies,$(1),$($($(1)_type)_native_toolchain) $($($(1)_type)_native_binutils) $($(1)_dependencies)))
$(foreach dep,$($(1)_all_dependencies),$(eval $(1)_build_id_deps+=$(dep)-$($(dep)_version)-$($(dep)_recipe_hash)))
$(eval $(1)_build_id_long:=$(1)-$($(1)_version)-$($(1)_recipe_hash)-$(release_type) $($(1)_build_id_deps) $($($(1)_type)_id))
$(eval $(1)_build_id:=$(shell echo -n "$($(1)_build_id_long)" | $(build_SHA256SUM) | cut -c-$(HASH_LENGTH)))
final_build_id_long+=$($(package)_build_id_long)

#compute package-specific paths
$(1)_build_subdir?=.
$(1)_download_file?=$($(1)_file_name)
$(1)_source_dir:=$(SOURCES_PATH)
$(1)_source:=$$($(1)_source_dir)/$($(1)_file_name)
$(1)_staging_dir=$(base_staging_dir)/$(host)/$(1)/$($(1)_version)-$($(1)_build_id)
$(1)_staging_prefix_dir:=$$($(1)_staging_dir)$($($(1)_type)_prefix)
$(1)_extract_dir:=$(base_build_dir)/$(host)/$(1)/$($(1)_version)-$($(1)_build_id)
$(1)_download_dir:=$(base_download_dir)/$(1)-$($(1)_version)
$(1)_build_dir:=$$($(1)_extract_dir)/$$($(1)_build_subdir)
$(1)_cached_checksum:=$(BASE_CACHE)/$(host)/$(1)/$(1)-$($(1)_version)-$($(1)_build_id).tar.gz.hash
$(1)_patch_dir:=$(base_build_dir)/$(host)/$(1)/$($(1)_version)-$($(1)_build_id)/.patches-$($(1)_build_id)
$(1)_prefixbin:=$($($(1)_type)_prefix)/bin/
$(1)_cached:=$(BASE_CACHE)/$(host)/$(1)/$(1)-$($(1)_version)-$($(1)_build_id).tar.gz
$(1)_build_log:=$(BASEDIR)/$(1)-$($(1)_version)-$($(1)_build_id).log
$(1)_all_sources=$($(1)_file_name) $($(1)_extra_sources)

#stamps
$(1)_fetched=$(SOURCES_PATH)/download-stamps/.stamp_fetched-$(1)-$($(1)_file_name).hash
$(1)_extracted=$$($(1)_extract_dir)/.stamp_extracted
$(1)_preprocessed=$$($(1)_extract_dir)/.stamp_preprocessed
$(1)_cleaned=$$($(1)_extract_dir)/.stamp_cleaned
$(1)_built=$$($(1)_build_dir)/.stamp_built
$(1)_configured=$(host_prefix)/.$(1)_stamp_configured
$(1)_staged=$$($(1)_staging_dir)/.stamp_staged
$(1)_postprocessed=$$($(1)_staging_prefix_dir)/.stamp_postprocessed
$(1)_download_path_fixed=$(subst :,\:,$$($(1)_download_path))


#default commands
# The default behavior for tar will try to set ownership when running as uid 0 and may not succeed, --no-same-owner disables this behavior
$(1)_fetch_cmds ?= $(call fetch_file,$(1),$(subst \:,:,$$($(1)_download_path_fixed)),$$($(1)_download_file),$($(1)_file_name),$($(1)_sha256_hash))
$(1)_extract_cmds ?= mkdir -p $$($(1)_extract_dir) && echo "$$($(1)_sha256_hash)  $$($(1)_source)" > $$($(1)_extract_dir)/.$$($(1)_file_name).hash &&  $(build_SHA256SUM) -c $$($(1)_extract_dir)/.$$($(1)_file_name).hash && $(build_TAR) --no-same-owner --strip-components=1 -xf $$($(1)_source)
$(1)_preprocess_cmds ?= true
$(1)_build_cmds ?= true
$(1)_config_cmds ?= true
$(1)_stage_cmds ?= true
$(1)_set_vars ?=


all_sources+=$$($(1)_fetched)
endef
#$(foreach dep_target,$($(1)_all_dependencies),$(eval $(1)_dependency_targets=$($(dep_target)_cached)))


define int_config_attach_build_config
$(eval $(call $(1)_set_vars,$(1)))
$(1)_cflags+=$($(1)_cflags_$(release_type))
$(1)_cflags+=$($(1)_cflags_$(host_arch)) $($(1)_cflags_$(host_arch)_$(release_type))
$(1)_cflags+=$($(1)_cflags_$(host_os)) $($(1)_cflags_$(host_os)_$(release_type))
$(1)_cflags+=$($(1)_cflags_$(host_arch)_$(host_os)) $($(1)_cflags_$(host_arch)_$(host_os)_$(release_type))

$(1)_cxxflags+=$($(1)_cxxflags_$(release_type))
$(1)_cxxflags+=$($(1)_cxxflags_$(host_arch)) $($(1)_cxxflags_$(host_arch)_$(release_type))
$(1)_cxxflags+=$($(1)_cxxflags_$(host_os)) $($(1)_cxxflags_$(host_os)_$(release_type))
$(1)_cxxflags+=$($(1)_cxxflags_$(host_arch)_$(host_os)) $($(1)_cxxflags_$(host_arch)_$(host_os)_$(release_type))

$(1)_cppflags+=$($(1)_cppflags_$(release_type))
$(1)_cppflags+=$($(1)_cppflags_$(host_arch)) $($(1)_cppflags_$(host_arch)_$(release_type))
$(1)_cppflags+=$($(1)_cppflags_$(host_os)) $($(1)_cppflags_$(host_os)_$(release_type))
$(1)_cppflags+=$($(1)_cppflags_$(host_arch)_$(host_os)) $($(1)_cppflags_$(host_arch)_$(host_os)_$(release_type))

$(1)_ldflags+=$($(1)_ldflags_$(release_type))
$(1)_ldflags+=$($(1)_ldflags_$(host_arch)) $($(1)_ldflags_$(host_arch)_$(release_type))
$(1)_ldflags+=$($(1)_ldflags_$(host_os)) $($(1)_ldflags_$(host_os)_$(release_type))
$(1)_ldflags+=$($(1)_ldflags_$(host_arch)_$(host_os)) $($(1)_ldflags_$(host_arch)_$(host_os)_$(release_type))

$(1)_build_opts+=$$($(1)_build_opts_$(release_type))
$(1)_build_opts+=$$($(1)_build_opts_$(host_arch)) $$($(1)_build_opts_$(host_arch)_$(release_type))
$(1)_build_opts+=$$($(1)_build_opts_$(host_os)) $$($(1)_build_opts_$(host_os)_$(release_type))
$(1)_build_opts+=$$($(1)_build_opts_$(host_arch)_$(host_os)) $$($(1)_build_opts_$(host_arch)_$(host_os)_$(release_type))

$(1)_config_opts+=$$($(1)_config_opts_$(release_type))
$(1)_config_opts+=$$($(1)_config_opts_$(host_arch)) $$($(1)_config_opts_$(host_arch)_$(release_type))
$(1)_config_opts+=$$($(1)_config_opts_$(host_os)) $$($(1)_config_opts_$(host_os)_$(release_type))
$(1)_config_opts+=$$($(1)_config_opts_$(host_arch)_$(host_os)) $$($(1)_config_opts_$(host_arch)_$(host_os)_$(release_type))

$(1)_config_env+=$$($(1)_config_env_$(release_type))
$(1)_config_env+=$($(1)_config_env_$(host_arch)) $($(1)_config_env_$(host_arch)_$(release_type))
$(1)_config_env+=$($(1)_config_env_$(host_os)) $($(1)_config_env_$(host_os)_$(release_type))
$(1)_config_env+=$($(1)_config_env_$(host_arch)_$(host_os)) $($(1)_config_env_$(host_arch)_$(host_os)_$(release_type))

$(1)_config_env+=PKG_CONFIG_LIBDIR=$($($(1)_type)_prefix)/lib/pkgconfig
$(1)_config_env+=PKG_CONFIG_PATH=$($($(1)_type)_prefix)/share/pkgconfig
$(1)_config_env+=PKG_CONFIG_SYSROOT_DIR=/
$(1)_config_env+=CMAKE_MODULE_PATH=$($($(1)_type)_prefix)/lib/cmake
$(1)_config_env+=PATH=$(build_prefix)/bin:$(PATH)
$(1)_build_env+=PATH=$(build_prefix)/bin:$(PATH)
$(1)_stage_env+=PATH=$(build_prefix)/bin:$(PATH)

# Setting a --build type that differs from --host will explicitly enable
# cross-compilation mode. Note that --build defaults to the output of
# config.guess, which is what we set it too here. This also quells autoconf
# warnings, "If you wanted to set the --build type, don't use --host.",
# when using versions older than 2.70.
$(1)_autoconf=./configure --build=$(BUILD) --host=$($($(1)_type)_host) --prefix=$($($(1)_type)_prefix) $$($(1)_config_opts) CC="$$($(1)_cc)" CXX="$$($(1)_cxx)"
ifneq ($($(1)_nm),)
$(1)_autoconf += NM="$$($(1)_nm)"
endif
ifneq ($($(1)_ranlib),)
$(1)_autoconf += RANLIB="$$($(1)_ranlib)"
endif
ifneq ($($(1)_ar),)
$(1)_autoconf += AR="$$($(1)_ar)"
endif
ifneq ($($(1)_cflags),)
$(1)_autoconf += CFLAGS="$$($(1)_cflags)"
endif
ifneq ($($(1)_cxxflags),)
$(1)_autoconf += CXXFLAGS="$$($(1)_cxxflags)"
endif
ifneq ($($(1)_cppflags),)
$(1)_autoconf += CPPFLAGS="$$($(1)_cppflags)"
endif
ifneq ($($(1)_ldflags),)
$(1)_autoconf += LDFLAGS="$$($(1)_ldflags)"
endif

$(1)_cmake=env CC="$$($(1)_cc)" \
               CFLAGS="$$($(1)_cppflags) $$($(1)_cflags)" \
               CXX="$$($(1)_cxx)" \
               CXXFLAGS="$$($(1)_cppflags) $$($(1)_cxxflags)" \
               LDFLAGS="$$($(1)_ldflags)" \
             cmake -DCMAKE_INSTALL_PREFIX:PATH="$$($($(1)_type)_prefix)" $$($(1)_config_opts)
ifeq ($($(1)_type),build)
$(1)_cmake += -DCMAKE_INSTALL_RPATH:PATH="$$($($(1)_type)_prefix)/lib"
else
ifneq ($(host),$(build))
$(1)_cmake += -DCMAKE_SYSTEM_NAME=$($(host_os)_cmake_system)
$(1)_cmake += -DCMAKE_C_COMPILER_TARGET=$(host)
$(1)_cmake += -DCMAKE_CXX_COMPILER_TARGET=$(host)
endif
endif
endef

define int_add_cmds
ifneq ($(LOG),)
$(1)_logging = >>$$($(1)_build_log) 2>&1 || { if test -f $$($(1)_build_log); then cat $$($(1)_build_log); fi; exit 1; }
endif

$($(1)_fetched):
	mkdir -p $$(@D) $(SOURCES_PATH)
	rm -f $$@
	touch $$@
	cd $$(@D); $($(1)_fetch_cmds)
	cd $($(1)_source_dir); $(foreach source,$($(1)_all_sources),$(build_SHA256SUM) $(source) >> $$(@);)
	touch $$@
$($(1)_extracted): | $($(1)_fetched)
	echo Extracting $(1)...
	mkdir -p $$(@D)
	cd $$(@D); $($(1)_extract_cmds)
	touch $$@
$($(1)_preprocessed): | $($(1)_extracted)
	echo Preprocessing $(1)...
	mkdir -p $$(@D) $($(1)_patch_dir)
	$(foreach patch,$($(1)_patches),cd $(PATCHES_PATH)/$(1); cp $(patch) $($(1)_patch_dir) ;)
	{ cd $$(@D); $($(1)_preprocess_cmds); } $$($(1)_logging)
	touch $$@
$($(1)_configured): | $($(1)_dependencies) $($(1)_preprocessed)
	echo Configuring $(1)...
	rm -rf $(host_prefix); mkdir -p $(host_prefix)/lib; cd $(host_prefix); $(foreach package,$($(1)_all_dependencies), $(build_TAR) --no-same-owner -xf $($(package)_cached); )
	mkdir -p $$($(1)_build_dir)
	+{ cd $$($(1)_build_dir); export $($(1)_config_env); $($(1)_config_cmds); } $$($(1)_logging)
	touch $$@
$($(1)_built): | $($(1)_configured)
	echo Building $(1)...
	mkdir -p $$(@D)
	+{ cd $$(@D); export $($(1)_build_env); $($(1)_build_cmds); } $$($(1)_logging)
	touch $$@
$($(1)_staged): | $($(1)_built)
	echo Staging $(1)...
	mkdir -p $($(1)_staging_dir)/$(host_prefix)
	+{ cd $($(1)_build_dir); export $($(1)_stage_env); $($(1)_stage_cmds); } $$($(1)_logging)
	rm -rf $($(1)_extract_dir)
	touch $$@
$($(1)_postprocessed): | $($(1)_staged)
	echo Postprocessing $(1)...
	cd $($(1)_staging_prefix_dir); $($(1)_postprocess_cmds)
	touch $$@
$($(1)_cached): | $($(1)_dependencies) $($(1)_postprocessed)
	echo Caching $(1)...
	cd $$($(1)_staging_dir)/$(host_prefix); \
	  find . ! -name '.stamp_postprocessed' -print0 | TZ=UTC xargs -0r touch -h -m -t 200001011200; \
	  find . ! -name '.stamp_postprocessed' | LC_ALL=C sort | $(build_TAR) --numeric-owner --no-recursion -czf $$($(1)_staging_dir)/$$(@F) -T -
	mkdir -p $$(@D)
	rm -rf $$(@D) && mkdir -p $$(@D)
	mv $$($(1)_staging_dir)/$$(@F) $$(@)
	rm -rf $($(1)_staging_dir)
	if test -f $($(1)_build_log); then mv $($(1)_build_log) $$(@D); fi
$($(1)_cached_checksum): $($(1)_cached)
	cd $$(@D); $(build_SHA256SUM) $$(<F) > $$(@)

.PHONY: $(1)
$(1): | $($(1)_cached_checksum)
.SECONDARY: $($(1)_cached) $($(1)_postprocessed) $($(1)_staged) $($(1)_built) $($(1)_configured) $($(1)_preprocessed) $($(1)_extracted) $($(1)_fetched)

endef

stages = fetched extracted preprocessed configured built staged postprocessed cached cached_checksum

define ext_add_stages
$(foreach stage,$(stages),
          $(1)_$(stage): $($(1)_$(stage))
          .PHONY: $(1)_$(stage))
endef

# These functions create the build targets for each package. They must be
# broken down into small steps so that each part is done for all packages
# before moving on to the next step. Otherwise, a package's info
# (build-id for example) would only be available to another package if it
# happened to be computed already.

#set the type for host/build packages.
$(foreach native_package,$(native_packages),$(eval $(native_package)_type=build))
$(foreach package,$(packages),$(eval $(package)_type=$(host_arch)_$(host_os)))

#set overridable defaults
$(foreach package,$(all_packages),$(eval $(call int_vars,$(package))))

#include package files
$(foreach native_package,$(native_packages),$(eval include packages/$(native_package).mk))
$(foreach package,$(packages),$(eval include packages/$(package).mk))

#compute a hash of all files that comprise this package's build recipe
$(foreach package,$(all_packages),$(eval $(call int_get_build_recipe_hash,$(package))))

#generate a unique id for this package, incorporating its dependencies as well
$(foreach package,$(all_packages),$(eval $(call int_get_build_id,$(package))))

#compute final vars after reading package vars
$(foreach package,$(all_packages),$(eval $(call int_config_attach_build_config,$(package))))

#create build targets
$(foreach package,$(all_packages),$(eval $(call int_add_cmds,$(package))))

#special exception: if a toolchain package exists, all non-native packages depend on it
$(foreach package,$(packages),$(eval $($(package)_extracted): |$($($(host_arch)_$(host_os)_native_toolchain)_cached) $($($(host_arch)_$(host_os)_native_binutils)_cached) ))
