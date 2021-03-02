sdk=$(shell xcrun --show-sdk-path)

build_darwin_CC:=clang --sysroot $(sdk)
build_darwin_CXX:=clang++ --sysroot $(sdk)
build_darwin_AR:=ar
build_darwin_RANLIB:=ranlib
build_darwin_STRIP:=strip
build_darwin_OTOOL:=otool
build_darwin_NM:=nm
build_darwin_INSTALL_NAME_TOOL:=install_name_tool
build_darwin_SHA256SUM=shasum -a 256
build_darwin_DOWNLOAD=curl --location --fail --connect-timeout $(DOWNLOAD_CONNECT_TIMEOUT) --retry $(DOWNLOAD_RETRIES) -o

#darwin host on darwin builder. overrides darwin host preferences.
darwin_CC=clang -mmacosx-version-min=$(OSX_MIN_VERSION) --sysroot $(sdk)
darwin_CXX:=clang++ -mmacosx-version-min=$(OSX_MIN_VERSION) -stdlib=libc++ --sysroot $(sdk)
darwin_AR:=ar
darwin_RANLIB:=ranlib
darwin_STRIP:=strip
darwin_LIBTOOL:=libtool
darwin_OTOOL:=otool
darwin_NM:=nm
darwin_INSTALL_NAME_TOOL:=install_name_tool
darwin_native_binutils=
darwin_native_toolchain=
