ANDROID_API_LEVEL ?= 26

android_toolchain_bin_dir := $(ANDROID_NDK)/toolchains/llvm/prebuilt/$(build_os)-$(build_arch)/bin

ifeq ($(HOST),armv7a-linux-android)
android_CXX := $(android_toolchain_bin_dir)/$(HOST)eabi$(ANDROID_API_LEVEL)-clang++
android_CC  := $(android_toolchain_bin_dir)/$(HOST)eabi$(ANDROID_API_LEVEL)-clang
android_libcxx_shared_dir := arm-linux-androideabi
else
android_CXX := $(android_toolchain_bin_dir)/$(HOST)$(ANDROID_API_LEVEL)-clang++
android_CC  := $(android_toolchain_bin_dir)/$(HOST)$(ANDROID_API_LEVEL)-clang
android_libcxx_shared_dir := $(host_arch)-linux-android
endif

android_CXXFLAGS := -std=$(CXX_STANDARD)
android_CFLAGS   := -std=$(C_STANDARD)

android_AR := $(android_toolchain_bin_dir)/llvm-ar
android_RANLIB := $(android_toolchain_bin_dir)/llvm-ranlib

android_cmake_system_name := Android
android_cmake_system_version := $(ANDROID_API_LEVEL)

# https://developer.android.com/ndk/guides/abis
ifeq ($(host_arch),armv7a)
  android_abi := armeabi-v7a
else ifeq ($(host_arch),aarch64)
  android_abi := arm64-v8a
else ifeq ($(host_arch),x86_64)
  android_abi := x86_64
endif
