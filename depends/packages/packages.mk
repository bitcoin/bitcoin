packages:=boost openssl gmp
native_packages := native_ccache native_comparisontool

qt_native_packages = native_protobuf
qt_packages = qt qrencode protobuf
qt_linux_packages=expat dbus libxcb xcb_proto libXau xproto freetype fontconfig libX11 xextproto libXext xtrans

wallet_packages=bdb

upnp_packages=miniupnpc

ifneq ($(build_os),darwin)
darwin_native_packages=native_libuuid native_openssl native_cctools native_cdrkit native_libdmg-hfsplus
endif
