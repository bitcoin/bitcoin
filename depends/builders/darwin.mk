tools=/Library/Developer/CommandLineTools/usr/bin
sdk=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk

build_darwin_CC:=$(tools)/clang --sysroot $(sdk)
build_darwin_CXX:=$(tools)/clang++ --sysroot $(sdk)
build_darwin_AR:=$(tools)/ar
build_darwin_RANLIB:=$(tools)/ranlib
build_darwin_STRIP:=$(tools)/strip
build_darwin_OTOOL:=$(tools)/otool
build_darwin_NM:=$(tools)/nm
build_darwin_INSTALL_NAME_TOOL:=$(tools)/install_name_tool
build_darwin_SHA256SUM=shasum -a 256
build_darwin_DOWNLOAD=curl --location --fail --connect-timeout $(DOWNLOAD_CONNECT_TIMEOUT) --retry $(DOWNLOAD_RETRIES) -o

#darwin host on darwin builder. overrides darwin host preferences.
darwin_CC=$(tools)/clang -mmacosx-version-min=$(OSX_MIN_VERSION) --sysroot $(sdk)
darwin_CXX:=$(tools)/clang++ -mmacosx-version-min=$(OSX_MIN_VERSION) -stdlib=libc++ --sysroot $(sdk)
darwin_AR:=$(tools)/ar
darwin_RANLIB:=$(tools)/ranlib
darwin_STRIP:=$(tools)/strip
darwin_LIBTOOL:=$(tools)/libtool
darwin_OTOOL:=$(tools)/otool
darwin_NM:=$(tools)/nm
darwin_INSTALL_NAME_TOOL:=$(tools)/install_name_tool
darwin_native_binutils=
darwin_native_toolchain=
