#pragma once
/**
	@file
	@brief int type definition and macros
	@author MITSUNARI Shigeo(@herumi)
*/

#if defined(_MSC_VER) && (MSC_VER <= 1500) && !defined(CYBOZU_DEFINED_INTXX)
	#define CYBOZU_DEFINED_INTXX
	typedef __int64 int64_t;
	typedef unsigned __int64 uint64_t;
	typedef unsigned int uint32_t;
	typedef int int32_t;
	typedef unsigned short uint16_t;
	typedef short int16_t;
	typedef unsigned char uint8_t;
	typedef signed char int8_t;
#else
	#include <stdint.h>
#endif

#ifdef _MSC_VER
	#ifndef CYBOZU_DEFINED_SSIZE_T
		#define CYBOZU_DEFINED_SSIZE_T
		#ifdef _WIN64
			typedef int64_t ssize_t;
		#else
			typedef int32_t ssize_t;
		#endif
	#endif
#else
	#include <unistd.h> // for ssize_t
#endif

#ifndef CYBOZU_ALIGN
	#ifdef _MSC_VER
		#define CYBOZU_ALIGN(x) __declspec(align(x))
	#else
		#define CYBOZU_ALIGN(x) __attribute__((aligned(x)))
	#endif
#endif
#ifndef CYBOZU_FORCE_INLINE
	#ifdef _MSC_VER
		#define CYBOZU_FORCE_INLINE __forceinline
	#else
		#define CYBOZU_FORCE_INLINE __attribute__((always_inline))
	#endif
#endif
#ifndef CYBOZU_UNUSED
	#ifdef __GNUC__
		#define CYBOZU_UNUSED __attribute__((unused))
	#else
		#define CYBOZU_UNUSED
	#endif
#endif
#ifndef CYBOZU_ALLOCA
	#ifdef _MSC_VER
		#include <malloc.h>
		#define CYBOZU_ALLOCA(x) _malloca(x)
	#else
		#define CYBOZU_ALLOCA(x) __builtin_alloca(x)
	#endif
#endif
#ifndef CYBOZU_NUM_OF_ARRAY
	#define CYBOZU_NUM_OF_ARRAY(x) (sizeof(x) / sizeof(*x))
#endif
#ifndef CYBOZU_SNPRINTF
	#if defined(_MSC_VER) && (_MSC_VER < 1900)
		#define CYBOZU_SNPRINTF(x, len, ...) (void)_snprintf_s(x, len, len - 1, __VA_ARGS__)
	#else
		#define CYBOZU_SNPRINTF(x, len, ...) (void)snprintf(x, len, __VA_ARGS__)
	#endif
#endif

// LLONG_MIN in limits.h is not defined in some env.
#define CYBOZU_LLONG_MIN (-9223372036854775807ll-1)

#define CYBOZU_CPP_VERSION_CPP03 0
#define CYBOZU_CPP_VERSION_TR1 1
#define CYBOZU_CPP_VERSION_CPP11 2
#define CYBOZU_CPP_VERSION_CPP14 3
#define CYBOZU_CPP_VERSION_CPP17 4

#ifdef __GNUC__
	#define CYBOZU_GNUC_PREREQ(major, minor) ((__GNUC__) * 100 + (__GNUC_MINOR__) >= (major) * 100 + (minor))
#else
	#define CYBOZU_GNUC_PREREQ(major, minor) 0
#endif

#if (__cplusplus >= 201703)
	#define CYBOZU_CPP_VERSION CYBOZU_CPP_VERSION_CPP17
#elif (__cplusplus >= 201402)
	#define CYBOZU_CPP_VERSION CYBOZU_CPP_VERSION_CPP14
#elif (__cplusplus >= 201103) || (defined(_MSC_VER) && _MSC_VER >= 1500) || defined(__GXX_EXPERIMENTAL_CXX0X__)
	#if defined(_MSC_VER) && (_MSC_VER <= 1600)
		#define CYBOZU_CPP_VERSION CYBOZU_CPP_VERSION_TR1
	#else
		#define CYBOZU_CPP_VERSION CYBOZU_CPP_VERSION_CPP11
	#endif
#elif CYBOZU_GNUC_PREREQ(4, 5) || (CYBOZU_GNUC_PREREQ(4, 2) && (defined(__GLIBCXX__) &&__GLIBCXX__ >= 20070719)) || defined(__INTEL_COMPILER) || (__clang_major__ >= 3)
	#define CYBOZU_CPP_VERSION CYBOZU_CPP_VERSION_TR1
#else
	#define CYBOZU_CPP_VERSION CYBOZU_CPP_VERSION_CPP03
#endif

#ifdef CYBOZU_USE_BOOST
	#define CYBOZU_NAMESPACE_STD boost
	#define CYBOZU_NAMESPACE_TR1_BEGIN
	#define CYBOZU_NAMESPACE_TR1_END
#elif (CYBOZU_CPP_VERSION == CYBOZU_CPP_VERSION_TR1) && !defined(__APPLE__)
	#define CYBOZU_NAMESPACE_STD std::tr1
	#define CYBOZU_NAMESPACE_TR1_BEGIN namespace tr1 {
	#define CYBOZU_NAMESPACE_TR1_END }
#else
	#define CYBOZU_NAMESPACE_STD std
	#define CYBOZU_NAMESPACE_TR1_BEGIN
	#define CYBOZU_NAMESPACE_TR1_END
#endif

#ifndef CYBOZU_OS_BIT
	#if defined(_WIN64) || defined(__x86_64__) || defined(__AARCH64EL__) || defined(__EMSCRIPTEN__) || defined(__LP64__)
		#define CYBOZU_OS_BIT 64
	#else
		#define CYBOZU_OS_BIT 32
	#endif
#endif

#ifndef CYBOZU_HOST
	#define CYBOZU_HOST_UNKNOWN 0
	#define CYBOZU_HOST_INTEL 1
	#define CYBOZU_HOST_ARM 2
	#if defined(_M_IX86) || defined(_M_AMD64) || defined(__x86_64__) || defined(__i386__)
		#define CYBOZU_HOST CYBOZU_HOST_INTEL
	#elif defined(__arm__) || defined(__AARCH64EL__)
		#define CYBOZU_HOST CYBOZU_HOST_ARM
	#else
		#define CYBOZU_HOST CYBOZU_HOST_UNKNOWN
	#endif
#endif

#ifndef CYBOZU_ENDIAN
	#define CYBOZU_ENDIAN_UNKNOWN 0
	#define CYBOZU_ENDIAN_LITTLE 1
	#define CYBOZU_ENDIAN_BIG 2
	#if (CYBOZU_HOST == CYBOZU_HOST_INTEL)
		#define CYBOZU_ENDIAN CYBOZU_ENDIAN_LITTLE
	#elif (CYBOZU_HOST == CYBOZU_HOST_ARM) && (defined(__ARM_EABI__) || defined(__AARCH64EL__))
		#define CYBOZU_ENDIAN CYBOZU_ENDIAN_LITTLE
	#elif defined(__s390x__)
		#define CYBOZU_ENDIAN CYBOZU_ENDIAN_BIG
	#else
		#define CYBOZU_ENDIAN CYBOZU_ENDIAN_UNKNOWN
	#endif
#endif

#if CYBOZU_CPP_VERSION >= CYBOZU_CPP_VERSION_CPP11
	#define CYBOZU_NOEXCEPT noexcept
	#define CYBOZU_NULLPTR nullptr
	#define CYBOZU_OVERRIDE override
	#define CYBOZU_FINAL final
#else
	#define CYBOZU_NOEXCEPT throw()
	#define CYBOZU_NULLPTR 0
	#define CYBOZU_OVERRIDE
	#define CYBOZU_FINAL
#endif
namespace cybozu {
template<class T>
void disable_warning_unused_variable(const T&) { }
template<class T, class S>
T cast(const S* ptr) { return static_cast<T>(static_cast<const void*>(ptr)); }
template<class T, class S>
T cast(S* ptr) { return static_cast<T>(static_cast<void*>(ptr)); }
} // cybozu
