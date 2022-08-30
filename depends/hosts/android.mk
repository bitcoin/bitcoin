ifeq ($(HOST),armv7a-linux-android)
android_CXX=$(ANDROID_TOOLCHAIN_BIN)/$(HOST)eabi$(ANDROID_API_LEVEL)-clang++
android_CC=$(ANDROID_TOOLCHAIN_BIN)/$(HOST)eabi$(ANDROID_API_LEVEL)-clang
android_NM=$(ANDROID_TOOLCHAIN_BIN)/$(HOST)eabi$(ANDROID_API_LEVEL)-nm
else
android_CXX=$(ANDROID_TOOLCHAIN_BIN)/$(HOST)$(ANDROID_API_LEVEL)-clang++
android_CC=$(ANDROID_TOOLCHAIN_BIN)/$(HOST)$(ANDROID_API_LEVEL)-clang
android_NM=$(ANDROID_TOOLCHAIN_BIN)/$(HOST)$(ANDROID_API_LEVEL)-nm
endif

android_CFLAGS=-std=$(C_STANDARD)
android_CXXFLAGS=-std=$(CXX_STANDARD)

ifneq ($(LTO),)
android_CFLAGS += -flto
android_LDFLAGS += -flto
endif

android_AR=$(ANDROID_TOOLCHAIN_BIN)/llvm-ar
android_RANLIB=$(ANDROID_TOOLCHAIN_BIN)/llvm-ranlib
android_NM=$(ANDROID_TOOLCHAIN_BIN)/llvm-nm

android_cmake_system=Android
