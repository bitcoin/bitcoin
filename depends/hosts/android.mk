ifeq ($(HOST),armv7a-linux-android)
android_AR=$(ANDROID_TOOLCHAIN_BIN)/arm-linux-androideabi-ar
android_CXX=$(ANDROID_TOOLCHAIN_BIN)/$(HOST)eabi$(ANDROID_API_LEVEL)-clang++
android_CC=$(ANDROID_TOOLCHAIN_BIN)/$(HOST)eabi$(ANDROID_API_LEVEL)-clang
android_RANLIB=$(ANDROID_TOOLCHAIN_BIN)/arm-linux-androideabi-ranlib
else
android_AR=$(ANDROID_TOOLCHAIN_BIN)/$(HOST)-ar
android_CXX=$(ANDROID_TOOLCHAIN_BIN)/$(HOST)$(ANDROID_API_LEVEL)-clang++
android_CC=$(ANDROID_TOOLCHAIN_BIN)/$(HOST)$(ANDROID_API_LEVEL)-clang
android_RANLIB=$(ANDROID_TOOLCHAIN_BIN)/$(HOST)-ranlib
endif
