packages:=boost openssl libevent
darwin_packages:=zeromq
linux_packages:=zeromq
native_packages := native_ccache native_comparisontool

qt_native_packages = native_protobuf
qt_packages = qrencode protobuf

qt_linux_packages= qt expat dbus libxcb xcb_proto libXau xproto freetype fontconfig libX11 xextproto libXext xtrans
qt_darwin_packages=qt
qt_mingw32_packages=qt


wallet_packages=bdb

upnp_packages=miniupnpc

ifneq ($(build_os),darwin)
darwin_native_packages=native_cctools native_cdrkit native_libdmg-hfsplus
endif
