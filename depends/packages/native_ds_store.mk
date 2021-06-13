package=native_ds_store
$(package)_version=1.3.0
$(package)_download_path=https://github.com/al45tair/ds_store/archive/
$(package)_file_name=v$($(package)_version).tar.gz
$(package)_sha256_hash=76b3280cd4e19e5179defa23fb594a9dd32643b0c80d774bd3108361d94fb46d
$(package)_install_libdir=$(build_prefix)/lib/python3/dist-packages

define $(package)_build_cmds
    python3 setup.py build
endef

define $(package)_stage_cmds
    mkdir -p $($(package)_install_libdir) && \
    python3 setup.py install --root=$($(package)_staging_dir) --prefix=$(build_prefix) --install-lib=$($(package)_install_libdir)
endef
packages/$(package).mk:: ;
