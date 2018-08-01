package=native_nsis
$(package)_version=3.03
$(package)_download_path=https://sourceforge.net/projects/nsis/files/NSIS%203/$($(package)_version)
$(package)_file_name=nsis-$($(package)_version)-src.tar.bz2
$(package)_sha256_hash=abae7f4488bc6de7a4dd760d5f0e7cd3aad7747d4d7cd85786697c8991695eaa
$(package)_dependencies=native_zlib

define $(package)_set_vars
$(package)_build_opts  = PREFIX=$(build_prefix)
$(package)_build_opts += PREFIX_DEST=$($(package)_staging_dir)
$(package)_build_opts += APPEND_CPPPATH=$(build_prefix)/include
$(package)_build_opts += APPEND_LIBPATH=$(build_prefix)/lib
$(package)_build_opts += SKIPDOC=COPYING
$(package)_build_opts += SKIPSTUBS=bzip2
$(package)_build_opts += SKIPPLUGINS=AdvSplash,Banner,BgImage,Dialer,InstallOptions,LangDLL,Library/TypeLib,Math,nsExec,NSISdl,Splash,UserInfo,VPatch/Source/Plugin
$(package)_build_opts += SKIPUTILS=Library/LibraryLocal,Library/RegTool,MakeLangId,Makensisw,'NSIS Menu',SubStart,VPatch/Source/GenPat,zip2exe
$(package)_build_opts += SKIPMISC=MultiUser,'Modern UI',VPatch
endef

define $(package)_build_cmds
  scons $($(package)_build_opts)
endef

define $(package)_stage_cmds
  scons $($(package)_build_opts) install-bin install-data
endef
