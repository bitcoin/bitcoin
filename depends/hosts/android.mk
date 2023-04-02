ifeq ($(HOST),armv7a-linux-android)
android_CXX=$(ANDROID_TOOLCHAIN_BIN)/$(HOST)eabi$(ANDROID_API_LEVEL)-clang++
android_CC=$(ANDROID_TOOLCHAIN_BIN)/$(HOST)eabi$(ANDROID_API_LEVEL)-clang
else
android_CXX=$(ANDROID_TOOLCHAIN_BIN)/$(HOST)$(ANDROID_API_LEVEL)-clang++
android_CC=$(ANDROID_TOOLCHAIN_BIN)/$(HOST)$(ANDROID_API_LEVEL)-clang
endif

android_CFLAGS=-std=$(C_STANDARD)
android_CXXFLAGS=-std=$(CXX_STANDARD)

ifneq ($(LTO),)
android_CFLAGS += -flto
android_LDFLAGS += -flto
endif

android_AR=$(ANDROID_TOOLCHAIN_BIN)/llvm-ar
android_RANLIB=$(ANDROID_TOOLCHAIN_BIN)/llvm-ranlib

android_cmake_system=Android

# https://developer.android.com/ndk/guides/abis
ifeq ($(host_arch),armv7a)
  android_abi = armeabi-v7a
else ifeq ($(host_arch),aarch64)
  android_abi = arm64-v8a
else ifeq ($(host_arch),x86_64)
  android_abi = x86_64
endif
