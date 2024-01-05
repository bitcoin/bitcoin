packages:=

boost_packages = boost

libevent_packages = libevent

qrencode_linux_packages = qrencode
qrencode_darwin_packages = qrencode
qrencode_mingw32_packages = qrencode

qt_linux_packages:=qt expat libxcb xcb_proto libXau xproto freetype fontconfig libxkbcommon libxcb_util libxcb_util_cursor libxcb_util_render libxcb_util_keysyms libxcb_util_image libxcb_util_wm
qt_darwin_packages=qt
qt_mingw32_packages=qt
ifneq ($(host),$(build))
qt_native_packages := native_qt
endif

sqlite_packages=sqlite

zmq_packages=zeromq

multiprocess_packages = capnp
multiprocess_native_packages = native_libmultiprocess native_capnp

usdt_linux_packages=systemtap
