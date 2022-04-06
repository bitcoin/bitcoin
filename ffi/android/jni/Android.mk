LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cpp .ll
LOCAL_MODULE := bls256
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
  LOCAL_BASE_LL := $(LOCAL_PATH)/../../../../../mcl/src/base64.ll
  LOCAL_CPP_OPT := -DMCL_SIZEOF_UNIT=8
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
  LOCAL_BASE_LL := $(LOCAL_PATH)/../../../../../mcl/src/base32.ll
  LOCAL_CPP_OPT := -DMCL_SIZEOF_UNIT=4
endif
LOCAL_SRC_FILES :=  $(LOCAL_PATH)/../../../../src/bls_c256.cpp $(LOCAL_PATH)/../../../../../mcl/src/fp.cpp $(LOCAL_BASE_LL)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../include $(LOCAL_PATH)/../../../../mcl/include
LOCAL_CPPFLAGS += -O3 -DNDEBUG -fPIC -DMCL_DONT_USE_XBYAK -DMCL_DONT_USE_OPENSSL -DMCL_LLVM_BMI2=0 -DMCL_USE_LLVM=1 -DMCL_USE_VINT $(LOCAL_CPP_OPT) -DMCL_VINT_FIXED_BUFFER -DMCL_MAX_BIT_SIZE=256 -DCYBOZU_DONT_USE_EXCEPTION -DCYBOZU_DONT_USE_STRING -fno-exceptions -fno-rtti -std=c++03
LOCAL_LDLIBS := -llog #-Wl,--no-warn-shared-textrel
include $(BUILD_SHARED_LIBRARY)
