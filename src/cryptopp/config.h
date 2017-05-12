// config.h - written and placed in the public domain by Wei Dai

//! \file config.h
//! \brief Library configuration file

#ifndef CRYPTOPP_CONFIG_H
#define CRYPTOPP_CONFIG_H

// ***************** Important Settings ********************

// define this if running on a big-endian CPU
#if !defined(IS_LITTLE_ENDIAN) && (defined(__BIG_ENDIAN__) || (defined(__s390__) || defined(__s390x__) || defined(__zarch__)) || (defined(__m68k__) || defined(__MC68K__)) || defined(__sparc) || defined(__sparc__) || defined(__hppa__) || defined(__MIPSEB__) || defined(__ARMEB__) || (defined(__MWERKS__) && !defined(__INTEL__)))
#	define IS_BIG_ENDIAN
#endif

// define this if running on a little-endian CPU
// big endian will be assumed if IS_LITTLE_ENDIAN is not defined
#ifndef IS_BIG_ENDIAN
#	define IS_LITTLE_ENDIAN
#endif

// Sanity checks. Some processors have more than big-, little- and bi-endian modes. PDP mode, where order results in "4312", should
//   raise red flags immediately. Additionally, mis-classified machines, like (previosuly) S/390, should raise red flags immediately.
#if defined(IS_BIG_ENDIAN) && defined(__GNUC__) && defined(__BYTE_ORDER__) && (__BYTE_ORDER__ != __ORDER_BIG_ENDIAN__)
# error "IS_BIG_ENDIAN is set, but __BYTE_ORDER__  does not equal __ORDER_BIG_ENDIAN__"
#endif
#if defined(IS_LITTLE_ENDIAN) && defined(__GNUC__) && defined(__BYTE_ORDER__) && (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__)
# error "IS_LITTLE_ENDIAN is set, but __BYTE_ORDER__  does not equal __ORDER_LITTLE_ENDIAN__"
#endif

// Define this if you want to disable all OS-dependent features,
// such as sockets and OS-provided random number generators
// #define NO_OS_DEPENDENCE

// Define this to use features provided by Microsoft's CryptoAPI.
// Currently the only feature used is Windows random number generation.
// This macro will be ignored if NO_OS_DEPENDENCE is defined.
// #define USE_MS_CRYPTOAPI

// Define this to use features provided by Microsoft's CryptoNG API.
// CryptoNG API is available in Vista and above and its cross platform,
// including desktop apps and store apps. Currently the only feature
// used is Windows random number generation.
// This macro will be ignored if NO_OS_DEPENDENCE is defined.
// #define USE_MS_CNGAPI

// If the user did not make a choice, then select CryptoNG if either
// Visual Studio 2015 is available, or Windows 10 or above is available.
#if !defined(USE_MS_CRYPTOAPI) && !defined(USE_MS_CNGAPI)
# if (_MSC_VER >= 1900) || ((WINVER >= 0x0A00 /*_WIN32_WINNT_WIN10*/) || (_WIN32_WINNT >= 0x0A00 /*_WIN32_WINNT_WIN10*/))
#  define USE_MS_CNGAPI
# else
#  define USE_MS_CRYPTOAPI
# endif
#endif

// Define this to ensure C/C++ standard compliance and respect for GCC aliasing rules and other alignment fodder. If you
// experience a break with GCC at -O3, you should try this first. Guard it in case its set on the command line (and it differs).
#ifndef CRYPTOPP_NO_UNALIGNED_DATA_ACCESS
# define CRYPTOPP_NO_UNALIGNED_DATA_ACCESS
#endif

// ***************** Less Important Settings ***************

// Library version
#define CRYPTOPP_VERSION 565

// Define this if you want to set a prefix for TestData/ and TestVectors/
//   Be mindful of the trailing slash since its simple concatenation.
//   g++ ... -DCRYPTOPP_DATA_DIR='"/tmp/cryptopp_test/share/"'
#ifndef CRYPTOPP_DATA_DIR
# define CRYPTOPP_DATA_DIR ""
#endif

// define this to retain (as much as possible) old deprecated function and class names
// #define CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY

// Define this to retain (as much as possible) ABI and binary compatibility with Crypto++ 5.6.2.
// Also see https://cryptopp.com/wiki/Config.h#Avoid_MAINTAIN_BACKWARDS_COMPATIBILITY
// #define CRYPTOPP_MAINTAIN_BACKWARDS_COMPATIBILITY_562

// Define this if you want or need the library's memcpy_s and memmove_s.
//   See http://github.com/weidai11/cryptopp/issues/28.
// #if !defined(CRYPTOPP_WANT_SECURE_LIB)
// # define CRYPTOPP_WANT_SECURE_LIB
// #endif

// File system code to write to GZIP archive.
#if !defined(GZIP_OS_CODE)
# define GZIP_OS_CODE 0
#endif

// Try this if your CPU has 256K internal cache or a slow multiply instruction
// and you want a (possibly) faster IDEA implementation using log tables
// #define IDEA_LARGECACHE

// Define this if, for the linear congruential RNG, you want to use
// the original constants as specified in S.K. Park and K.W. Miller's
// CACM paper.
// #define LCRNG_ORIGINAL_NUMBERS

// Define this if you want Integer's operator<< to honor std::showbase (and
// std::noshowbase). If defined, Integer will use a suffix of 'b', 'o', 'h'
// or '.' (the last for decimal) when std::showbase is in effect. If
// std::noshowbase is set, then the suffix is not added to the Integer. If
// not defined, existing behavior is preserved and Integer will use a suffix
// of 'b', 'o', 'h' or '.' (the last for decimal).
// #define CRYPTOPP_USE_STD_SHOWBASE

// choose which style of sockets to wrap (mostly useful for MinGW which has both)
#if !defined(NO_BERKELEY_STYLE_SOCKETS) && !defined(PREFER_BERKELEY_STYLE_SOCKETS)
# define PREFER_BERKELEY_STYLE_SOCKETS
#endif

// #if !defined(NO_WINDOWS_STYLE_SOCKETS) && !defined(PREFER_WINDOWS_STYLE_SOCKETS)
// # define PREFER_WINDOWS_STYLE_SOCKETS
// #endif

// set the name of Rijndael cipher, was "Rijndael" before version 5.3
#define CRYPTOPP_RIJNDAEL_NAME "AES"

// CRYPTOPP_DEBUG enables the library's CRYPTOPP_ASSERT. CRYPTOPP_ASSERT
//   raises a SIGTRAP (Unix) or calls DebugBreak() (Windows). CRYPTOPP_ASSERT
//   is only in effect when CRYPTOPP_DEBUG, DEBUG or _DEBUG is defined. Unlike
//   Posix assert, CRYPTOPP_ASSERT is not affected by NDEBUG (or failure to
//   define it).
//   Also see http://github.com/weidai11/cryptopp/issues/277, CVE-2016-7420
#if (defined(DEBUG) || defined(_DEBUG)) && !defined(CRYPTOPP_DEBUG)
# define CRYPTOPP_DEBUG 1
#endif

// ***************** Initialization and Constructor priorities ********************

// MacPorts/GCC and Solaris/GCC does not provide constructor(priority). Apple/GCC and Fink/GCC do provide it.
// See http://cryptopp.com/wiki/Static_Initialization_Order_Fiasco

// CRYPTOPP_INIT_PRIORITY attempts to manage initialization of C++ static objects.
// Under GCC, the library uses init_priority attribute in the range
// [CRYPTOPP_INIT_PRIORITY, CRYPTOPP_INIT_PRIORITY+100]. Under Windows,
// CRYPTOPP_INIT_PRIORITY enlists "#pragma init_seg(lib)".
#ifndef CRYPTOPP_INIT_PRIORITY
# define CRYPTOPP_INIT_PRIORITY 250
#endif

// CRYPTOPP_USER_PRIORITY is for other libraries and user code that is using Crypto++
// and managing C++ static object creation. It is guaranteed not to conflict with
// values used by (or would be used by) the Crypto++ library.
#if defined(CRYPTOPP_INIT_PRIORITY) && (CRYPTOPP_INIT_PRIORITY > 0)
# define CRYPTOPP_USER_PRIORITY (CRYPTOPP_INIT_PRIORITY + 101)
#else
# define CRYPTOPP_USER_PRIORITY 350
#endif

// __attribute__(init_priority(250)) is supported
#if (__GNUC__ && (CRYPTOPP_INIT_PRIORITY > 0) && ((CRYPTOPP_GCC_VERSION >= 40300) || (CRYPTOPP_LLVM_CLANG_VERSION >= 20900) || (_INTEL_COMPILER >= 300)) && !(MACPORTS_GCC_COMPILER > 0) && !defined(__sun__))
# define HAVE_GCC_CONSTRUCTOR1 1
#endif

// __attribute__(init_priority()) is supported
#if (__GNUC__ && (CRYPTOPP_INIT_PRIORITY > 0) && !HAVE_GCC_CONSTRUCTOR1 && !(MACPORTS_GCC_COMPILER > 0) && !defined(__sun__))
# define HAVE_GCC_CONSTRUCTOR0 1
#endif

#if (_MSC_VER && (CRYPTOPP_INIT_PRIORITY > 0))
# define HAVE_MSC_INIT_PRIORITY 1
#endif

// ***************** Important Settings Again ********************
// But the defaults should be ok.

// namespace support is now required
#ifdef NO_NAMESPACE
#	error namespace support is now required
#endif

// Define this to workaround a Microsoft CryptoAPI bug where
// each call to CryptAcquireContext causes a 100 KB memory leak.
// Defining this will cause Crypto++ to make only one call to CryptAcquireContext.
#define WORKAROUND_MS_BUG_Q258000

#ifdef CRYPTOPP_DOXYGEN_PROCESSING
// Document the namespce exists. Put it here before CryptoPP is undefined below.
//! \namespace CryptoPP
//! \brief Crypto++ library namespace
//! \details Nearly all classes are located in the CryptoPP namespace. Within
//!   the namespace, there are two additional namespaces.
//!   <ul>
//!     <li>Name - namespace for names used with \p NameValuePairs and documented in argnames.h
//!     <li>Weak - namespace for weak and wounded algorithms, like ARC4, MD5 and Pananma
//!   </ul>
namespace CryptoPP { }
// Bring in the symbols fund in the weak namespace; and fold Weak1 into Weak
#		define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#		define Weak1 Weak
// Avoid putting "CryptoPP::" in front of everything in Doxygen output
#       define CryptoPP
#       define NAMESPACE_BEGIN(x)
#       define NAMESPACE_END
// Get Doxygen to generate better documentation for these typedefs
#       define DOCUMENTED_TYPEDEF(x, y) class y : public x {};
// Make "protected" "private" so the functions and members are not documented
#		define protected private
#else
#       define NAMESPACE_BEGIN(x) namespace x {
#       define NAMESPACE_END }
#       define DOCUMENTED_TYPEDEF(x, y) typedef x y;
#endif
#define ANONYMOUS_NAMESPACE_BEGIN namespace {
#define ANONYMOUS_NAMESPACE_END }
#define USING_NAMESPACE(x) using namespace x;
#define DOCUMENTED_NAMESPACE_BEGIN(x) namespace x {
#define DOCUMENTED_NAMESPACE_END }

// What is the type of the third parameter to bind?
// For Unix, the new standard is ::socklen_t (typically unsigned int), and the old standard is int.
// Unfortunately there is no way to tell whether or not socklen_t is defined.
// To work around this, TYPE_OF_SOCKLEN_T is a macro so that you can change it from the makefile.
#ifndef TYPE_OF_SOCKLEN_T
#	if defined(_WIN32) || defined(__CYGWIN__)
#		define TYPE_OF_SOCKLEN_T int
#	else
#		define TYPE_OF_SOCKLEN_T ::socklen_t
#	endif
#endif

#if defined(__CYGWIN__) && defined(PREFER_WINDOWS_STYLE_SOCKETS)
#	define __USE_W32_SOCKETS
#endif

typedef unsigned char byte;		// put in global namespace to avoid ambiguity with other byte typedefs

NAMESPACE_BEGIN(CryptoPP)

typedef unsigned short word16;
typedef unsigned int word32;

#if defined(_MSC_VER) || defined(__BORLANDC__)
	typedef unsigned __int64 word64;
	#define W64LIT(x) x##ui64
#elif (_LP64 || __LP64__)
	typedef unsigned long word64;
	#define W64LIT(x) x##UL
#else
	typedef unsigned long long word64;
	#define W64LIT(x) x##ULL
#endif

// define large word type, used for file offsets and such
typedef word64 lword;
const lword LWORD_MAX = W64LIT(0xffffffffffffffff);

// Clang pretends to be VC++, too.
//   See http://github.com/weidai11/cryptopp/issues/147
#if defined(_MSC_VER) && defined(__clang__)
# error: "Unsupported configuration"
#endif

#ifdef __GNUC__
	#define CRYPTOPP_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

// Apple and LLVM's Clang. Apple Clang version 7.0 roughly equals LLVM Clang version 3.7
#if defined(__clang__ ) && !defined(__apple_build_version__)
	#define CRYPTOPP_LLVM_CLANG_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
	#define CRYPTOPP_CLANG_INTEGRATED_ASSEMBLER 1
#elif defined(__clang__ ) && defined(__apple_build_version__)
	#define CRYPTOPP_APPLE_CLANG_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
	#define CRYPTOPP_CLANG_INTEGRATED_ASSEMBLER 1
#endif

#ifdef _MSC_VER
	#define CRYPTOPP_MSC_VERSION (_MSC_VER)
#endif

// Need GCC 4.6/Clang 1.7/Apple Clang 2.0 or above due to "GCC diagnostic {push|pop}"
#if (CRYPTOPP_GCC_VERSION >= 40600) || (CRYPTOPP_LLVM_CLANG_VERSION >= 10700) || (CRYPTOPP_APPLE_CLANG_VERSION >= 20000)
	#define CRYPTOPP_GCC_DIAGNOSTIC_AVAILABLE 1
#endif

// Clang due to "Inline assembly operands don't work with .intel_syntax", http://llvm.org/bugs/show_bug.cgi?id=24232
//   TODO: supply the upper version when LLVM fixes it. We set it to 20.0 for compilation purposes.
#if (defined(CRYPTOPP_LLVM_CLANG_VERSION) && CRYPTOPP_LLVM_CLANG_VERSION <= 200000) || (defined(CRYPTOPP_APPLE_CLANG_VERSION) && CRYPTOPP_APPLE_CLANG_VERSION <= 200000) || defined(CRYPTOPP_CLANG_INTEGRATED_ASSEMBLER)
	#define CRYPTOPP_DISABLE_INTEL_ASM 1
#endif

// define hword, word, and dword. these are used for multiprecision integer arithmetic
// Intel compiler won't have _umul128 until version 10.0. See http://softwarecommunity.intel.com/isn/Community/en-US/forums/thread/30231625.aspx
#if (defined(_MSC_VER) && (!defined(__INTEL_COMPILER) || __INTEL_COMPILER >= 1000) && (defined(_M_X64) || defined(_M_IA64))) || (defined(__DECCXX) && defined(__alpha__)) || (defined(__INTEL_COMPILER) && defined(__x86_64__)) || (defined(__SUNPRO_CC) && defined(__x86_64__))
	typedef word32 hword;
	typedef word64 word;
#else
	#define CRYPTOPP_NATIVE_DWORD_AVAILABLE 1
	#if defined(__alpha__) || defined(__ia64__) || defined(_ARCH_PPC64) || defined(__x86_64__) || defined(__mips64) || defined(__sparc64__)
		#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !(CRYPTOPP_GCC_VERSION == 40001 && defined(__APPLE__)) && CRYPTOPP_GCC_VERSION >= 30400
			// GCC 4.0.1 on MacOS X is missing __umodti3 and __udivti3
			// mode(TI) division broken on amd64 with GCC earlier than GCC 3.4
			typedef word32 hword;
			typedef word64 word;
			typedef __uint128_t dword;
			typedef __uint128_t word128;
			#define CRYPTOPP_WORD128_AVAILABLE 1
		#else
			// if we're here, it means we're on a 64-bit CPU but we don't have a way to obtain 128-bit multiplication results
			typedef word16 hword;
			typedef word32 word;
			typedef word64 dword;
		#endif
	#else
		// being here means the native register size is probably 32 bits or less
		#define CRYPTOPP_BOOL_SLOW_WORD64 1
		typedef word16 hword;
		typedef word32 word;
		typedef word64 dword;
	#endif
#endif
#ifndef CRYPTOPP_BOOL_SLOW_WORD64
	#define CRYPTOPP_BOOL_SLOW_WORD64 0
#endif

const unsigned int WORD_SIZE = sizeof(word);
const unsigned int WORD_BITS = WORD_SIZE * 8;

NAMESPACE_END

#ifndef CRYPTOPP_L1_CACHE_LINE_SIZE
	// This should be a lower bound on the L1 cache line size. It's used for defense against timing attacks.
	// Also see http://stackoverflow.com/questions/794632/programmatically-get-the-cache-line-size.
	#if defined(_M_X64) || defined(__x86_64__) || (__arm64__) || (__aarch64__)
		#define CRYPTOPP_L1_CACHE_LINE_SIZE 64
	#else
		// L1 cache line size is 32 on Pentium III and earlier
		#define CRYPTOPP_L1_CACHE_LINE_SIZE 32
	#endif
#endif

#if defined(_MSC_VER)
	#if _MSC_VER == 1200
		#include <malloc.h>
	#endif
	#if _MSC_VER > 1200 || defined(_mm_free)
		#define CRYPTOPP_MSVC6PP_OR_LATER		// VC 6 processor pack or later
	#else
		#define CRYPTOPP_MSVC6_NO_PP			// VC 6 without processor pack
	#endif
#endif

#ifndef CRYPTOPP_ALIGN_DATA
	#if defined(CRYPTOPP_MSVC6PP_OR_LATER)
		#define CRYPTOPP_ALIGN_DATA(x) __declspec(align(x))
	#elif defined(__GNUC__)
		#define CRYPTOPP_ALIGN_DATA(x) __attribute__((aligned(x)))
	#else
		#define CRYPTOPP_ALIGN_DATA(x)
	#endif
#endif

#ifndef CRYPTOPP_SECTION_ALIGN16
#if defined(__GNUC__) && !defined(__APPLE__)
		// the alignment attribute doesn't seem to work without this section attribute when -fdata-sections is turned on
		#define CRYPTOPP_SECTION_ALIGN16 __attribute__((section ("CryptoPP_Align16")))
	#else
		#define CRYPTOPP_SECTION_ALIGN16
	#endif
#endif

// The section attribute attempts to initialize CPU flags to avoid Valgrind findings above -O1
#if ((__MACH__ >= 1) && ((CRYPTOPP_LLVM_CLANG_VERSION >= 30600) || (CRYPTOPP_APPLE_CLANG_VERSION >= 70100) || (CRYPTOPP_GCC_VERSION >= 40300)))
	#define CRYPTOPP_SECTION_INIT __attribute__((section ("__DATA,__data")))
#elif ((__ELF__ >= 1) && (CRYPTOPP_GCC_VERSION >= 40300))
	#define CRYPTOPP_SECTION_INIT __attribute__((section ("nocommon")))
#else
	#define CRYPTOPP_SECTION_INIT
#endif

#if defined(_MSC_VER) || defined(__fastcall)
	#define CRYPTOPP_FASTCALL __fastcall
#else
	#define CRYPTOPP_FASTCALL
#endif

// VC60 workaround: it doesn't allow typename in some places
#if defined(_MSC_VER) && (_MSC_VER < 1300)
#define CPP_TYPENAME
#else
#define CPP_TYPENAME typename
#endif

// VC60 workaround: can't cast unsigned __int64 to float or double
#if defined(_MSC_VER) && !defined(CRYPTOPP_MSVC6PP_OR_LATER)
#define CRYPTOPP_VC6_INT64 (__int64)
#else
#define CRYPTOPP_VC6_INT64
#endif

#ifdef _MSC_VER
#define CRYPTOPP_NO_VTABLE __declspec(novtable)
#else
#define CRYPTOPP_NO_VTABLE
#endif

#ifdef _MSC_VER
	// 4127: conditional expression is constant
	// 4231: nonstandard extension used : 'extern' before template explicit instantiation
	// 4250: dominance
	// 4251: member needs to have dll-interface
	// 4275: base needs to have dll-interface
	// 4505: unreferenced local function
	// 4512: assignment operator not generated
	// 4660: explicitly instantiating a class that's already implicitly instantiated
	// 4661: no suitable definition provided for explicit template instantiation request
	// 4786: identifer was truncated in debug information
	// 4355: 'this' : used in base member initializer list
	// 4910: '__declspec(dllexport)' and 'extern' are incompatible on an explicit instantiation
#	pragma warning(disable: 4127 4231 4250 4251 4275 4505 4512 4660 4661 4786 4355 4910)
	// Security related, possible defects
	// http://blogs.msdn.com/b/vcblog/archive/2010/12/14/off-by-default-compiler-warnings-in-visual-c.aspx
#	pragma warning(once: 4191 4242 4263 4264 4266 4302 4826 4905 4906 4928)
#endif

#ifdef __BORLANDC__
// 8037: non-const function called for const object. needed to work around BCB2006 bug
#	pragma warn -8037
#endif

// [GCC Bug 53431] "C++ preprocessor ignores #pragma GCC diagnostic". Clang honors it.
#if CRYPTOPP_GCC_DIAGNOSTIC_AVAILABLE
# pragma GCC diagnostic ignored "-Wunknown-pragmas"
# pragma GCC diagnostic ignored "-Wunused-function"
#endif

// You may need to force include a C++ header on Android when using STLPort to ensure
// _STLPORT_VERSION is defined: CXXFLAGS="-DNDEBUG -g2 -O2 -std=c++11 -include iosfwd"
// TODO: Figure out C++17 and lack of std::uncaught_exception
#if (defined(_MSC_VER) && _MSC_VER <= 1300) || defined(__MWERKS__) || (defined(_STLPORT_VERSION) && ((_STLPORT_VERSION < 0x450) || defined(_STLP_NO_UNCAUGHT_EXCEPT_SUPPORT)))
#define CRYPTOPP_DISABLE_UNCAUGHT_EXCEPTION
#endif

#ifndef CRYPTOPP_DISABLE_UNCAUGHT_EXCEPTION
#define CRYPTOPP_UNCAUGHT_EXCEPTION_AVAILABLE
#endif

#ifdef CRYPTOPP_DISABLE_X86ASM		// for backwards compatibility: this macro had both meanings
#define CRYPTOPP_DISABLE_ASM
#define CRYPTOPP_DISABLE_SSE2
#endif

// Apple's Clang prior to 5.0 cannot handle SSE2 (and Apple does not use LLVM Clang numbering...)
#if defined(CRYPTOPP_APPLE_CLANG_VERSION) && (CRYPTOPP_APPLE_CLANG_VERSION < 50000)
# define CRYPTOPP_DISABLE_ASM
#endif

// Sun Studio 12 provides GCC inline assembly, http://blogs.oracle.com/x86be/entry/gcc_style_asm_inlining_support
// We can enable SSE2 for Sun Studio in the makefile with -D__SSE2__, but users may not compile with it.
#if !defined(CRYPTOPP_DISABLE_ASM) && !defined(__SSE2__) && defined(__x86_64__) && (__SUNPRO_CC >= 0x5100)
# define __SSE2__ 1
#endif

#if !defined(CRYPTOPP_DISABLE_ASM) && ((defined(_MSC_VER) && defined(_M_IX86)) || (defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))))
	// C++Builder 2010 does not allow "call label" where label is defined within inline assembly
	#define CRYPTOPP_X86_ASM_AVAILABLE

	#if !defined(CRYPTOPP_DISABLE_SSE2) && (defined(CRYPTOPP_MSVC6PP_OR_LATER) || CRYPTOPP_GCC_VERSION >= 30300 || defined(__SSE2__))
		#define CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE 1
	#else
		#define CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE 0
	#endif

	#if !defined(CRYPTOPP_DISABLE_SSE3) && (_MSC_VER >= 1500 || (defined(__SSE3__) && defined(__SSSE3__)))
		#define CRYPTOPP_BOOL_SSSE3_ASM_AVAILABLE 1
	#else
		#define CRYPTOPP_BOOL_SSSE3_ASM_AVAILABLE 0
	#endif
#endif

#if !defined(CRYPTOPP_DISABLE_ASM) && defined(_MSC_VER) && defined(_M_X64)
	#define CRYPTOPP_X64_MASM_AVAILABLE
#endif

#if !defined(CRYPTOPP_DISABLE_ASM) && defined(__GNUC__) && defined(__x86_64__)
	#define CRYPTOPP_X64_ASM_AVAILABLE
#endif

#if !defined(CRYPTOPP_DISABLE_ASM) && (defined(CRYPTOPP_MSVC6PP_OR_LATER) || defined(__SSE2__)) && !defined(_M_ARM)
	#define CRYPTOPP_BOOL_SSE2_INTRINSICS_AVAILABLE 1
#else
	#define CRYPTOPP_BOOL_SSE2_INTRINSICS_AVAILABLE 0
#endif

// Intrinsics availible in GCC 4.3 (http://gcc.gnu.org/gcc-4.3/changes.html) and
//   MSVC 2008 (http://msdn.microsoft.com/en-us/library/bb892950%28v=vs.90%29.aspx)
//   SunCC could generate SSE4 at 12.1, but the intrinsics are missing until 12.4.
#if !defined(CRYPTOPP_DISABLE_ASM) && !defined(CRYPTOPP_DISABLE_SSE4) && !defined(_M_ARM) && ((_MSC_VER >= 1500) || (defined(__SSE4_1__) && defined(__SSE4_2__)))
	#define CRYPTOPP_BOOL_SSE4_INTRINSICS_AVAILABLE 1
#else
	#define CRYPTOPP_BOOL_SSE4_INTRINSICS_AVAILABLE 0
#endif

// Don't disgorge AES-NI from CLMUL. There will be two to four subtle breaks
#if !defined(CRYPTOPP_DISABLE_ASM) && !defined(CRYPTOPP_DISABLE_AESNI) && !defined(_M_ARM) && (_MSC_FULL_VER >= 150030729 || __INTEL_COMPILER >= 1110 || (defined(__AES__) && defined(__PCLMUL__)))
	#define CRYPTOPP_BOOL_AESNI_INTRINSICS_AVAILABLE 1
#else
	#define CRYPTOPP_BOOL_AESNI_INTRINSICS_AVAILABLE 0
#endif

// AVX2 in MSC 18.00
#if !defined(CRYPTOPP_DISABLE_ASM) && !defined(CRYPTOPP_DISABLE_AVX) && !defined(_M_ARM) && ((_MSC_VER >= 1600) || (defined(__RDRND__) || defined(__RDSEED__) || defined(__AVX__)))
	#define CRYPTOPP_BOOL_AVX_AVAILABLE 1
#else
	#define CRYPTOPP_BOOL_AVX_AVAILABLE 0
#endif

// Requires ARMv7 and ACLE 1.0. Testing shows ARMv7 is really ARMv7a under most toolchains.
#if !defined(CRYPTOPP_BOOL_NEON_INTRINSICS_AVAILABLE) && !defined(CRYPTOPP_DISABLE_ASM)
# if defined(__ARM_NEON__) || defined(__ARM_NEON) || defined(_M_ARM)
#  define CRYPTOPP_BOOL_NEON_INTRINSICS_AVAILABLE 1
# endif
#endif

// Requires ARMv8 and ACLE 2.0. For GCC, requires 4.8 and above.
// Microsoft plans to support ARM-64, but its not clear how to detect it.
// TODO: Add MSC_VER and ARM-64 platform define when available
#if !defined(CRYPTOPP_BOOL_ARM_CRC32_INTRINSICS_AVAILABLE) && !defined(CRYPTOPP_DISABLE_ASM)
# if defined(__ARM_FEATURE_CRC32) || defined(_M_ARM64)
#  define CRYPTOPP_BOOL_ARM_CRC32_INTRINSICS_AVAILABLE 1
# endif
#endif

// Requires ARMv8 and ACLE 2.0. For GCC, requires 4.8 and above.
// Microsoft plans to support ARM-64, but its not clear how to detect it.
// TODO: Add MSC_VER and ARM-64 platform define when available
#if !defined(CRYPTOPP_BOOL_ARM_CRYPTO_INTRINSICS_AVAILABLE) && !defined(CRYPTOPP_DISABLE_ASM)
# if defined(__ARM_FEATURE_CRYPTO) || defined(_M_ARM64)
#  define CRYPTOPP_BOOL_ARM_CRYPTO_INTRINSICS_AVAILABLE 1
# endif
#endif

#if CRYPTOPP_BOOL_SSE2_INTRINSICS_AVAILABLE || CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE || CRYPTOPP_BOOL_NEON_INTRINSICS_AVAILABLE || defined(CRYPTOPP_X64_MASM_AVAILABLE)
	#define CRYPTOPP_BOOL_ALIGN16 1
#else
	#define CRYPTOPP_BOOL_ALIGN16 0
#endif

// how to allocate 16-byte aligned memory (for SSE2)
#if defined(CRYPTOPP_MSVC6PP_OR_LATER)
	#define CRYPTOPP_MM_MALLOC_AVAILABLE
#elif defined(__APPLE__)
	#define CRYPTOPP_APPLE_MALLOC_AVAILABLE
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
	#define CRYPTOPP_MALLOC_ALIGNMENT_IS_16
#elif defined(__linux__) || defined(__sun__) || defined(__CYGWIN__)
	#define CRYPTOPP_MEMALIGN_AVAILABLE
#else
	#define CRYPTOPP_NO_ALIGNED_ALLOC
#endif

// Apple always provides 16-byte aligned, and tells us to use calloc
// http://developer.apple.com/library/mac/documentation/Performance/Conceptual/ManagingMemory/Articles/MemoryAlloc.html

// how to disable inlining
#if defined(_MSC_VER) && _MSC_VER >= 1300
#	define CRYPTOPP_NOINLINE_DOTDOTDOT
#	define CRYPTOPP_NOINLINE __declspec(noinline)
#elif defined(__GNUC__)
#	define CRYPTOPP_NOINLINE_DOTDOTDOT
#	define CRYPTOPP_NOINLINE __attribute__((noinline))
#else
#	define CRYPTOPP_NOINLINE_DOTDOTDOT ...
#	define CRYPTOPP_NOINLINE
#endif

// How to declare class constants
// Use enum for OS X 10.5 ld, http://github.com/weidai11/cryptopp/issues/255
#if (defined(_MSC_VER) && _MSC_VER <= 1300) || defined(__INTEL_COMPILER) || defined(__BORLANDC__)
#	define CRYPTOPP_CONSTANT(x) enum {x};
#else
#	define CRYPTOPP_CONSTANT(x) static const int x;
#endif

// Linux provides X32, which is 32-bit integers, longs and pointers on x86_64 using the full x86_64 register set.
// Detect via __ILP32__ (http://wiki.debian.org/X32Port). However, __ILP32__ shows up in more places than
// the System V ABI specs calls out, like on just about any 32-bit system with Clang.
#if ((__ILP32__ >= 1) || (_ILP32 >= 1)) && defined(__x86_64__)
	#define CRYPTOPP_BOOL_X32 1
#else
	#define CRYPTOPP_BOOL_X32 0
#endif

// see http://predef.sourceforge.net/prearch.html
#if (defined(_M_IX86) || defined(__i386__) || defined(__i386) || defined(_X86_) || defined(__I86__) || defined(__INTEL__)) && !CRYPTOPP_BOOL_X32
	#define CRYPTOPP_BOOL_X86 1
#else
	#define CRYPTOPP_BOOL_X86 0
#endif

#if (defined(_M_X64) || defined(__x86_64__)) && !CRYPTOPP_BOOL_X32
	#define CRYPTOPP_BOOL_X64 1
#else
	#define CRYPTOPP_BOOL_X64 0
#endif

// Undo the ASM and Intrinsic related defines due to X32.
#if CRYPTOPP_BOOL_X32
# undef CRYPTOPP_BOOL_X64
# undef CRYPTOPP_X64_ASM_AVAILABLE
# undef CRYPTOPP_X64_MASM_AVAILABLE
#endif

#if defined(__arm__) || defined(__aarch32__) || defined(_M_ARM)
	#define CRYPTOPP_BOOL_ARM32 1
#else
	#define CRYPTOPP_BOOL_ARM32 0
#endif

// Microsoft plans to support ARM-64, but its not clear how to detect it.
// TODO: Add MSC_VER and ARM-64 platform define when available
#if defined(__arm64__) || defined(__aarch64__) || defined(_M_ARM64)
	#define CRYPTOPP_BOOL_ARM64 1
#else
	#define CRYPTOPP_BOOL_ARM64 0
#endif

#if !defined(CRYPTOPP_NO_UNALIGNED_DATA_ACCESS) && !defined(CRYPTOPP_ALLOW_UNALIGNED_DATA_ACCESS)
#if (CRYPTOPP_BOOL_X64 || CRYPTOPP_BOOL_X86 || CRYPTOPP_BOOL_X32 || defined(__powerpc__) || (__ARM_FEATURE_UNALIGNED >= 1))
	#define CRYPTOPP_ALLOW_UNALIGNED_DATA_ACCESS
#endif
#endif

// ***************** determine availability of OS features ********************

#ifndef NO_OS_DEPENDENCE

#if defined(_WIN32) || defined(__CYGWIN__)
#define CRYPTOPP_WIN32_AVAILABLE
#endif

#if defined(__unix__) || defined(__MACH__) || defined(__NetBSD__) || defined(__sun)
#define CRYPTOPP_UNIX_AVAILABLE
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#define CRYPTOPP_BSD_AVAILABLE
#endif

#if defined(CRYPTOPP_WIN32_AVAILABLE) || defined(CRYPTOPP_UNIX_AVAILABLE)
#	define HIGHRES_TIMER_AVAILABLE
#endif

#ifdef CRYPTOPP_WIN32_AVAILABLE
# if !defined(WINAPI_FAMILY)
#	define THREAD_TIMER_AVAILABLE
# elif defined(WINAPI_FAMILY)
#   if (WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
#	  define THREAD_TIMER_AVAILABLE
#  endif
# endif
#endif

#ifdef CRYPTOPP_UNIX_AVAILABLE
#	define HAS_BERKELEY_STYLE_SOCKETS
#	define SOCKETS_AVAILABLE
#endif

// Sockets are only available under Windows Runtime desktop partition apps (despite the MSDN literature)
#ifdef CRYPTOPP_WIN32_AVAILABLE
# define HAS_WINDOWS_STYLE_SOCKETS
# if !defined(WINAPI_FAMILY)
#	define SOCKETS_AVAILABLE
# elif defined(WINAPI_FAMILY)
#   if (WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
#	  define SOCKETS_AVAILABLE
#   endif
# endif
#endif

#if defined(HAS_WINDOWS_STYLE_SOCKETS) && (!defined(HAS_BERKELEY_STYLE_SOCKETS) || defined(PREFER_WINDOWS_STYLE_SOCKETS))
#	define USE_WINDOWS_STYLE_SOCKETS
#else
#	define USE_BERKELEY_STYLE_SOCKETS
#endif

#if defined(CRYPTOPP_WIN32_AVAILABLE) && defined(SOCKETS_AVAILABLE) && !defined(USE_BERKELEY_STYLE_SOCKETS)
#	define WINDOWS_PIPES_AVAILABLE
#endif

#if defined(CRYPTOPP_UNIX_AVAILABLE) || defined(CRYPTOPP_DOXYGEN_PROCESSING)
#	define NONBLOCKING_RNG_AVAILABLE
#	define BLOCKING_RNG_AVAILABLE
#	define OS_RNG_AVAILABLE
#	define HAS_PTHREADS
#	define THREADS_AVAILABLE
#endif

#if defined(CRYPTOPP_BSD_AVAILABLE) || defined(CRYPTOPP_UNIX_AVAILABLE) || defined(__CYGWIN__)
# define UNIX_SIGNALS_AVAILABLE 1
#endif

#ifdef CRYPTOPP_WIN32_AVAILABLE
# if !defined(WINAPI_FAMILY)
#	define HAS_WINTHREADS
#	define THREADS_AVAILABLE
#	define NONBLOCKING_RNG_AVAILABLE
#	define OS_RNG_AVAILABLE
# elif defined(WINAPI_FAMILY)
#   if (WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
#	  define HAS_WINTHREADS
#	  define THREADS_AVAILABLE
#	  define NONBLOCKING_RNG_AVAILABLE
#	  define OS_RNG_AVAILABLE
#   elif !(WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
#     if ((WINVER >= 0x0A00 /*_WIN32_WINNT_WIN10*/) || (_WIN32_WINNT >= 0x0A00 /*_WIN32_WINNT_WIN10*/))
#	    define NONBLOCKING_RNG_AVAILABLE
#	    define OS_RNG_AVAILABLE
#     endif
#   endif
# endif
#endif

#endif	// NO_OS_DEPENDENCE

// ***************** DLL related ********************

#if defined(CRYPTOPP_WIN32_AVAILABLE) && !defined(CRYPTOPP_DOXYGEN_PROCESSING)

#ifdef CRYPTOPP_EXPORTS
#define CRYPTOPP_IS_DLL
#define CRYPTOPP_DLL __declspec(dllexport)
#elif defined(CRYPTOPP_IMPORTS)
#define CRYPTOPP_IS_DLL
#define CRYPTOPP_DLL __declspec(dllimport)
#else
#define CRYPTOPP_DLL
#endif

#define CRYPTOPP_API __cdecl

#else	// not CRYPTOPP_WIN32_AVAILABLE

#define CRYPTOPP_DLL
#define CRYPTOPP_API

#endif	// CRYPTOPP_WIN32_AVAILABLE

#if defined(__MWERKS__)
#define CRYPTOPP_EXTERN_DLL_TEMPLATE_CLASS extern class CRYPTOPP_DLL
#elif defined(__BORLANDC__) || defined(__SUNPRO_CC)
#define CRYPTOPP_EXTERN_DLL_TEMPLATE_CLASS template class CRYPTOPP_DLL
#else
#define CRYPTOPP_EXTERN_DLL_TEMPLATE_CLASS extern template class CRYPTOPP_DLL
#endif

#if defined(CRYPTOPP_MANUALLY_INSTANTIATE_TEMPLATES) && !defined(CRYPTOPP_IMPORTS)
#define CRYPTOPP_DLL_TEMPLATE_CLASS template class CRYPTOPP_DLL
#else
#define CRYPTOPP_DLL_TEMPLATE_CLASS CRYPTOPP_EXTERN_DLL_TEMPLATE_CLASS
#endif

#if defined(__MWERKS__)
#define CRYPTOPP_EXTERN_STATIC_TEMPLATE_CLASS extern class
#elif defined(__BORLANDC__) || defined(__SUNPRO_CC)
#define CRYPTOPP_EXTERN_STATIC_TEMPLATE_CLASS template class
#else
#define CRYPTOPP_EXTERN_STATIC_TEMPLATE_CLASS extern template class
#endif

#if defined(CRYPTOPP_MANUALLY_INSTANTIATE_TEMPLATES) && !defined(CRYPTOPP_EXPORTS)
#define CRYPTOPP_STATIC_TEMPLATE_CLASS template class
#else
#define CRYPTOPP_STATIC_TEMPLATE_CLASS CRYPTOPP_EXTERN_STATIC_TEMPLATE_CLASS
#endif

// ************** Unused variable ***************

// Portable way to suppress warnings.
//   Moved from misc.h due to circular depenedencies.
#define CRYPTOPP_UNUSED(x) ((void)(x))

// ************** Deprecated ***************

#if (CRYPTOPP_GCC_VERSION >= 40500) || (CRYPTOPP_LLVM_CLANG_VERSION >= 20800)
# define CRYPTOPP_DEPRECATED(msg) __attribute__((deprecated (msg)));
#elif (CRYPTOPP_GCC_VERSION)
# define CRYPTOPP_DEPRECATED(msg) __attribute__((deprecated));
#else
# define CRYPTOPP_DEPRECATED(msg)
#endif

// ***************** C++11 related ********************

// Visual Studio began at VS2010, http://msdn.microsoft.com/en-us/library/hh567368%28v=vs.110%29.aspx.
// Intel and C++11 language features, http://software.intel.com/en-us/articles/c0x-features-supported-by-intel-c-compiler
// GCC and C++11 language features, http://gcc.gnu.org/projects/cxx0x.html
// Clang and C++11 language features, http://clang.llvm.org/cxx_status.html
#if ((_MSC_VER >= 1600) || (__cplusplus >= 201103L)) && !defined(_STLPORT_VERSION)
# define CRYPTOPP_CXX11 1
#endif

// Hack ahead. Apple's standard library does not have C++'s unique_ptr in C++11. We can't
//   test for unique_ptr directly because some of the non-Apple Clangs on OS X fail the same
//   way. However, modern standard libraries have <forward_list>, so we test for it instead.
//   Thanks to Jonathan Wakely for devising the clever test for modern/ancient versions.
// TODO: test under Xcode 3, where g++ is really g++.
#if defined(__APPLE__) && defined(__clang__)
#  if !(defined(__has_include) && __has_include(<forward_list>))
#    undef CRYPTOPP_CXX11
#  endif
#endif

// C++11 or C++14 is available
#if defined(CRYPTOPP_CXX11)

// atomics: MS at VS2012 (17.00); GCC at 4.4; Clang at 3.1/3.2; Intel 13.0; SunCC 12.5.
#if (CRYPTOPP_MSC_VERSION >= 1700)
#  define CRYPTOPP_CXX11_ATOMICS 1
#elif (__INTEL_COMPILER >= 1300)
#  define CRYPTOPP_CXX11_ATOMICS 1
#elif defined(__clang__)
#  if __has_feature(cxx_atomic)
#    define CRYPTOPP_CXX11_ATOMICS 1
#  endif
#elif (CRYPTOPP_GCC_VERSION >= 40400)
#  define CRYPTOPP_CXX11_ATOMICS 1
#elif (__SUNPRO_CC >= 0x5140)
#  define CRYPTOPP_CXX11_ATOMICS 1
#endif // atomics

// synchronization: MS at VS2012 (17.00); GCC at 4.4; Clang at 3.3; Xcode 5.0; Intel 12.0; SunCC 12.4.
// TODO: verify Clang and Intel versions; find __has_feature(x) extension for Clang
#if (CRYPTOPP_MSC_VERSION >= 1700)
#  define CRYPTOPP_CXX11_SYNCHRONIZATION 1
#elif (__INTEL_COMPILER >= 1200)
#  define CRYPTOPP_CXX11_SYNCHRONIZATION 1
#elif (CRYPTOPP_LLVM_CLANG_VERSION >= 30300) || (CRYPTOPP_APPLE_CLANG_VERSION >= 50000)
#  define CRYPTOPP_CXX11_SYNCHRONIZATION 1
#elif (CRYPTOPP_GCC_VERSION >= 40400)
#  define CRYPTOPP_CXX11_SYNCHRONIZATION 1
#elif (__SUNPRO_CC >= 0x5130)
#  define CRYPTOPP_CXX11_SYNCHRONIZATION 1
#endif // synchronization

// alignof/alignas: MS at VS2015 (19.00); GCC at 4.8; Clang at 3.3; Intel 15.0; SunCC 12.4.
#if (CRYPTOPP_MSC_VERSION >= 1900)
#  define CRYPTOPP_CXX11_ALIGNAS 1
#  define CRYPTOPP_CXX11_ALIGNOF 1
#elif (__INTEL_COMPILER >= 1500)
#  define CRYPTOPP_CXX11_ALIGNAS 1
#  define CRYPTOPP_CXX11_ALIGNOF 1
#elif defined(__clang__)
#  if __has_feature(cxx_alignas)
#  define CRYPTOPP_CXX11_ALIGNAS 1
#  endif
#  if __has_feature(cxx_alignof)
#  define CRYPTOPP_CXX11_ALIGNOF 1
#  endif
#elif (CRYPTOPP_GCC_VERSION >= 40800)
#  define CRYPTOPP_CXX11_ALIGNAS 1
#  define CRYPTOPP_CXX11_ALIGNOF 1
#elif (__SUNPRO_CC >= 0x5130)
#  define CRYPTOPP_CXX11_ALIGNAS 1
#  define CRYPTOPP_CXX11_ALIGNOF 1
#endif // alignof/alignas

// noexcept: MS at VS2015 (19.00); GCC at 4.6; Clang at 3.0; Intel 14.0; SunCC 12.4.
#if (CRYPTOPP_MSC_VERSION >= 1900)
#  define CRYPTOPP_CXX11_NOEXCEPT 1
#elif (__INTEL_COMPILER >= 1400)
#  define CRYPTOPP_CXX11_NOEXCEPT 1
#elif defined(__clang__)
#  if __has_feature(cxx_noexcept)
#    define CRYPTOPP_CXX11_NOEXCEPT 1
#  endif
#elif (CRYPTOPP_GCC_VERSION >= 40600)
#  define CRYPTOPP_CXX11_NOEXCEPT 1
#elif (__SUNPRO_CC >= 0x5130)
#  define CRYPTOPP_CXX11_NOEXCEPT 1
#endif // noexcept compilers

// variadic templates: MS at VS2013 (18.00); GCC at 4.3; Clang at 2.9; Intel 12.1; SunCC 12.4.
#if (CRYPTOPP_MSC_VERSION >= 1800)
#  define CRYPTOPP_CXX11_VARIADIC_TEMPLATES 1
#elif (__INTEL_COMPILER >= 1210)
#  define CRYPTOPP_CXX11_VARIADIC_TEMPLATES 1
#elif defined(__clang__)
#  if __has_feature(cxx_variadic_templates)
#    define CRYPTOPP_CXX11_VARIADIC_TEMPLATES 1
#  endif
#elif (CRYPTOPP_GCC_VERSION >= 40300)
#  define CRYPTOPP_CXX11_VARIADIC_TEMPLATES 1
#elif (__SUNPRO_CC >= 0x5130)
#  define CRYPTOPP_CXX11_VARIADIC_TEMPLATES 1
#endif // variadic templates

// constexpr: MS at VS2015 (19.00); GCC at 4.6; Clang at 3.0; Intel 16.0; SunCC 12.4.
// Intel has mis-supported the feature since at least ICPC 13.00
#if (CRYPTOPP_MSC_VERSION >= 1900)
#  define CRYPTOPP_CXX11_CONSTEXPR 1
#elif (__INTEL_COMPILER >= 1600)
#  define CRYPTOPP_CXX11_CONSTEXPR 1
#elif defined(__clang__)
#  if __has_feature(cxx_constexpr)
#    define CRYPTOPP_CXX11_CONSTEXPR 1
#  endif
#elif (CRYPTOPP_GCC_VERSION >= 40600)
#  define CRYPTOPP_CXX11_CONSTEXPR 1
#elif (__SUNPRO_CC >= 0x5130)
#  define CRYPTOPP_CXX11_CONSTEXPR 1
#endif // constexpr compilers

// TODO: Emplacement, R-values and Move semantics
// Needed because we are catching warnings with GCC and MSC

#endif // CRYPTOPP_CXX11

#if defined(CRYPTOPP_CXX11_NOEXCEPT)
#  define CRYPTOPP_THROW noexcept(false)
#  define CRYPTOPP_NO_THROW noexcept(true)
#else
#  define CRYPTOPP_THROW
#  define CRYPTOPP_NO_THROW
#endif // CRYPTOPP_CXX11_NOEXCEPT

#if defined(CRYPTOPP_CXX11_CONSTEXPR)
#  define CRYPTOPP_CONSTEXPR constexpr
#else
#  define CRYPTOPP_CONSTEXPR
#endif // CRYPTOPP_CXX11_CONSTEXPR

// Hack... CRYPTOPP_ALIGN_DATA is defined earlier, before C++11 alignas availability is determined
#if defined(CRYPTOPP_CXX11_ALIGNAS)
# undef CRYPTOPP_ALIGN_DATA
# define CRYPTOPP_ALIGN_DATA(x) alignas(x)
#endif  // CRYPTOPP_CXX11_ALIGNAS

// Hack... CRYPTOPP_CONSTANT is defined earlier, before C++11 constexpr availability is determined
#if defined(CRYPTOPP_CXX11_CONSTEXPR)
# undef CRYPTOPP_CONSTANT
# define CRYPTOPP_CONSTANT(x) constexpr static int x;
#endif

// OK to comment the following out, but please report it so we can fix it.
// C++17 value taken from http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4567.pdf.
#if (defined(__cplusplus) && (__cplusplus >= 199711L) && (__cplusplus < 201402L)) && !defined(CRYPTOPP_UNCAUGHT_EXCEPTION_AVAILABLE)
# error "std::uncaught_exception is not available. This is likely a configuration error."
#endif

#endif
