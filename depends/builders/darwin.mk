build_darwin_CC:=$(shell xcrun -f clang) -isysroot$(shell xcrun --show-sdk-path)
build_darwin_CXX:=$(shell xcrun -f clang++) -isysroot$(shell xcrun --show-sdk-path)
build_darwin_AR:=$(shell xcrun -f ar)
build_darwin_RANLIB:=$(shell xcrun -f ranlib)
build_darwin_STRIP:=$(shell xcrun -f strip)
build_darwin_OBJDUMP:=$(shell xcrun -f objdump)
build_darwin_NM:=$(shell xcrun -f nm)
build_darwin_DSYMUTIL:=$(shell xcrun -f dsymutil)
build_darwin_SHA256SUM=shasum -a 256
build_darwin_DOWNLOAD=curl --location --fail --connect-timeout $(DOWNLOAD_CONNECT_TIMEOUT) --retry $(DOWNLOAD_RETRIES) -o

#darwin host on darwin builder. overrides darwin host preferences.
darwin_CC=$(shell xcrun -f clang) -isysroot$(shell xcrun --show-sdk-path)
darwin_CXX:=$(shell xcrun -f clang++) -stdlib=libc++ -isysroot$(shell xcrun --show-sdk-path)
darwin_AR:=$(shell xcrun -f ar)
darwin_RANLIB:=$(shell xcrun -f ranlib)
darwin_STRIP:=$(shell xcrun -f strip)
darwin_OBJDUMP:=$(shell xcrun -f objdump)
darwin_NM:=$(shell xcrun -f nm)
darwin_DSYMUTIL:=$(shell xcrun -f dsymutil)

x86_64_darwin_CFLAGS += -arch x86_64
x86_64_darwin_CXXFLAGS += -arch x86_64
aarch64_darwin_CFLAGS += -arch arm64
aarch64_darwin_CXXFLAGS += -arch arm64
