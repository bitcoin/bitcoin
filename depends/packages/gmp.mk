package=gmp
$(package)_version=6.3.0
$(package)_download_path=https://mirrors.kernel.org/gnu/gmp/
$(package)_file_name=gmp-$($(package)_version).tar.xz
$(package)_sha256_hash=a3c2b80201b89e68616f4ad30bc66aee4927c3ce50e33929ca819d5c43538898

define $(package)_set_vars
$(package)_config_opts+=--enable-cxx --enable-fat --with-pic --disable-shared
endef
