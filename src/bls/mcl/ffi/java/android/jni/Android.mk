LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cpp .ll .cxx
LOCAL_MODULE := mcljava

LOCAL_MCL_DIR := $(LOCAL_PATH)/../../../../

ifeq ($(TARGET_ARCH_ABI),x86_64)
  MY_BIT := 64
endif
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
  MY_BIT := 64
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
  MY_BIT := 32
endif
ifeq ($(TARGET_ARCH_ABI),x86)
  MY_BIT := 32
endif
ifeq ($(MY_BIT),64)
  MY_BASE_LL := $(LOCAL_MCL_DIR)/src/base64.ll
  LOCAL_CPPFLAGS += -DMCL_SIZEOF_UNIT=8
endif
ifeq ($(MY_BIT),32)
  MY_BASE_LL := $(LOCAL_MCL_DIR)/src/base32.ll
  LOCAL_CPPFLAGS += -DMCL_SIZEOF_UNIT=4
endif
LOCAL_SRC_FILES := $(LOCAL_MCL_DIR)/ffi/java/mcl_wrap.cxx $(LOCAL_MCL_DIR)/src/bn_c384_256.cpp $(LOCAL_MCL_DIR)/src/fp.cpp $(MY_BASE_LL)
LOCAL_C_INCLUDES := $(LOCAL_MCL_DIR)/include $(LOCAL_MCL_DIR)/src $(LOCAL_MCL_DIR)/ffi/java
LOCAL_CPPFLAGS += -DMCL_DONT_USE_XBYAK
LOCAL_CPPFLAGS += -O3 -DNDEBUG -fPIC -DMCL_DONT_USE_OPENSSL -DMCL_LLVM_BMI2=0 -DMCL_USE_LLVM=1 -DMCL_USE_VINT -DMCL_VINT_FIXED_BUFFER -DMCL_MAX_BIT_SIZE=384
LOCAL_CPPFLAGS += -fno-threadsafe-statics
#LOCAL_CPPFLAGS+=-fno-exceptions -fno-rtti -DCYBOZU_DONT_USE_EXCEPTION -DCYBOZU_DONT_USE_STRING -std=c++03

#LOCAL_CPPFLAGS += -DBLS_ETH
#LOCAL_LDLIBS := -llog #-Wl,--no-warn-shared-textrel
#include $(BUILD_STATIC_LIBRARY)
include $(BUILD_SHARED_LIBRARY)
