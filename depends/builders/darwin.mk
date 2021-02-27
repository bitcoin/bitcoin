build_darwin_CC:=$(shell clang) --sysroot $(shell xcrun --show-sdk-path)
build_darwin_CXX:=$(shell clang++) --sysroot $(shell xcrun --show-sdk-path)
build_darwin_AR:=$(shell ar)
build_darwin_RANLIB:=$(shell ranlib)
build_darwin_STRIP:=$(shell strip)
build_darwin_OTOOL:=$(shell otool)
build_darwin_NM:=$(shell nm)
build_darwin_INSTALL_NAME_TOOL:=$(shell install_name_tool)
build_darwin_SHA256SUM=shasum -a 256
build_darwin_DOWNLOAD=curl --location --fail --connect-timeout $(DOWNLOAD_CONNECT_TIMEOUT) --retry $(DOWNLOAD_RETRIES) -o

#darwin host on darwin builder. overrides darwin host preferences.
darwin_CC=$(shell clang) -mmacosx-version-min=$(OSX_MIN_VERSION) --sysroot $(shell xcrun --show-sdk-path)
darwin_CXX:=$(shell clang++) -mmacosx-version-min=$(OSX_MIN_VERSION) -stdlib=libc++ --sysroot $(shell xcrun --show-sdk-path)
darwin_AR:=$(shell ar)
darwin_RANLIB:=$(shell ranlib)
darwin_STRIP:=$(shell  strip)
darwin_LIBTOOL:=$(shell libtool)
darwin_OTOOL:=$(shell otool)
darwin_NM:=$(shell nm)
darwin_INSTALL_NAME_TOOL:=$(install_name_tool)
darwin_native_binutils=
darwin_native_toolchain=
