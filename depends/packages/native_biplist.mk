package=native_biplist
$(package)_version=0.9
$(package)_download_path=https://pypi.python.org/packages/source/b/biplist
$(package)_file_name=biplist-$($(package)_version).tar.gz
$(package)_sha256_hash=b57cadfd26e4754efdf89e9e37de87885f9b5c847b2615688ca04adfaf6ca604
$(package)_install_libdir=$(build_prefix)/lib/python/dist-packages
$(package)_patches=sorted_list.patch

define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/sorted_list.patch
endef

define $(package)_build_cmds
    python3 setup.py build
endef

define $(package)_stage_cmds
    mkdir -p $($(package)_install_libdir) && \
    python3 setup.py install --root=$($(package)_staging_dir) --prefix=$(build_prefix) --install-lib=$($(package)_install_libdir)
endef
