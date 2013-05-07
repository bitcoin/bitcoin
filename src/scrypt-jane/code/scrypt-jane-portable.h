/* determine os */
#if defined(_WIN32)	|| defined(_WIN64) || defined(__TOS_WIN__) || defined(__WINDOWS__)
	#include <windows.h>
	#include <wincrypt.h>
	#define OS_WINDOWS
#elif defined(sun) || defined(__sun) || defined(__SVR4) || defined(__svr4__)
	#include <sys/mman.h>
	#include <sys/time.h>
	#include <fcntl.h>

	#define OS_SOLARIS
#else
	#include <sys/mman.h>
	#include <sys/time.h>
	#include <sys/param.h> /* need this to define BSD */
	#include <unistd.h>
	#include <fcntl.h>

	#define OS_NIX
	#if defined(__linux__)
		#include <endian.h>
		#define OS_LINUX
	#elif defined(BSD)
		#define OS_BSD

		#if defined(MACOS_X) || (defined(__APPLE__) & defined(__MACH__))
			#define OS_OSX
		#elif defined(macintosh) || defined(Macintosh)
			#define OS_MAC
		#elif defined(__OpenBSD__)
			#define OS_OPENBSD
		#endif
	#endif
#endif


/* determine compiler */
#if defined(_MSC_VER)
	#define COMPILER_MSVC _MSC_VER
	#if ((COMPILER_MSVC > 1200) || defined(_mm_free))
		#define COMPILER_MSVC6PP_AND_LATER
	#endif
	#if (COMPILER_MSVC >= 1500)
		#define COMPILER_HAS_TMMINTRIN
	#endif
	
	#pragma warning(disable : 4127) /* conditional expression is constant */
	#pragma warning(disable : 4100) /* unreferenced formal parameter */
	
	#define _CRT_SECURE_NO_WARNINGS	
	#include <float.h>
	#include <stdlib.h> /* _rotl */
	#include <intrin.h>

	typedef unsigned char uint8_t;
	typedef unsigned short uint16_t;
	typedef unsigned int uint32_t;
	typedef signed int int32_t;	
	typedef unsigned __int64 uint64_t;
	typedef signed __int64 int64_t;

	#define ROTL32(a,b) _rotl(a,b)
	#define ROTR32(a,b) _rotr(a,b)
	#define ROTL64(a,b) _rotl64(a,b)
	#define ROTR64(a,b) _rotr64(a,b)
	#undef NOINLINE
	#define NOINLINE __declspec(noinline)
	#undef INLINE
	#define INLINE __forceinline
	#undef FASTCALL
	#define FASTCALL __fastcall
	#undef CDECL
	#define CDECL __cdecl
	#undef STDCALL
	#define STDCALL __stdcall
	#undef NAKED
	#define NAKED __declspec(naked)
	#define MM16 __declspec(align(16))
#endif
#if defined(__ICC)
	#define COMPILER_INTEL
#endif
#if defined(__GNUC__)
	#if (__GNUC__ >= 3)
		#define COMPILER_GCC_PATCHLEVEL __GNUC_PATCHLEVEL__
	#else
		#define COMPILER_GCC_PATCHLEVEL 0
	#endif
	#define COMPILER_GCC (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + COMPILER_GCC_PATCHLEVEL)
	#define ROTL32(a,b) (((a) << (b)) | ((a) >> (32 - b)))
	#define ROTR32(a,b) (((a) >> (b)) | ((a) << (32 - b)))
	#define ROTL64(a,b) (((a) << (b)) | ((a) >> (64 - b)))
	#define ROTR64(a,b) (((a) >> (b)) | ((a) << (64 - b)))
	#undef NOINLINE
	#if (COMPILER_GCC >= 30000)
		#define NOINLINE __attribute__((noinline))
	#else
		#define NOINLINE
	#endif
	#undef INLINE
	#if (COMPILER_GCC >= 30000)
		#define INLINE __attribute__((always_inline))
	#else
		#define INLINE inline
	#endif
	#undef FASTCALL
	#if (COMPILER_GCC >= 30400)
		#define FASTCALL __attribute__((fastcall))
	#else
		#define FASTCALL
	#endif
	#undef CDECL
	#define CDECL __attribute__((cdecl))
	#undef STDCALL
	#define STDCALL __attribute__((stdcall))
	#define MM16 __attribute__((aligned(16)))
	#include <stdint.h>
#endif
#if defined(__MINGW32__) || defined(__MINGW64__)
	#define COMPILER_MINGW
#endif
#if defined(__PATHCC__)
	#define COMPILER_PATHCC
#endif

#define OPTIONAL_INLINE
#if defined(OPTIONAL_INLINE)
	#undef OPTIONAL_INLINE
	#define OPTIONAL_INLINE INLINE
#else
	#define OPTIONAL_INLINE
#endif

#define CRYPTO_FN NOINLINE STDCALL

/* determine cpu */
#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__ ) || defined(_M_X64)
	#define CPU_X86_64
#elif defined(__i586__) || defined(__i686__) || (defined(_M_IX86) && (_M_IX86 >= 500))
	#define CPU_X86 500
#elif defined(__i486__) || (defined(_M_IX86) && (_M_IX86 >= 400))
	#define CPU_X86 400
#elif defined(__i386__) || (defined(_M_IX86) && (_M_IX86 >= 300)) || defined(__X86__) || defined(_X86_) || defined(__I86__)
	#define CPU_X86 300
#elif defined(__ia64__) || defined(_IA64) || defined(__IA64__) || defined(_M_IA64) || defined(__ia64)
	#define CPU_IA64
#endif

#if defined(__sparc__) || defined(__sparc) || defined(__sparcv9)
	#define CPU_SPARC
	#if defined(__sparcv9)
		#define CPU_SPARC64
	#endif
#endif

#if defined(CPU_X86_64) || defined(CPU_IA64) || defined(CPU_SPARC64) || defined(__64BIT__) || defined(__LP64__) || defined(_LP64) || (defined(_MIPS_SZLONG) && (_MIPS_SZLONG == 64))
	#define CPU_64BITS
	#undef FASTCALL
	#define FASTCALL
	#undef CDECL
	#define CDECL
	#undef STDCALL
	#define STDCALL
#endif

#if defined(powerpc) || defined(__PPC__) || defined(__ppc__) || defined(_ARCH_PPC) || defined(__powerpc__) || defined(__powerpc) || defined(POWERPC) || defined(_M_PPC)
	#define CPU_PPC
	#if defined(_ARCH_PWR7)
		#define CPU_POWER7
	#elif defined(__64BIT__)
		#define CPU_PPC64
	#else
		#define CPU_PPC32
	#endif
#endif

#if defined(__hppa__) || defined(__hppa)
	#define CPU_HPPA
#endif

#if defined(__alpha__) || defined(__alpha) || defined(_M_ALPHA)
	#define CPU_ALPHA
#endif

/* endian */

#if ((defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && (__BYTE_ORDER == __LITTLE_ENDIAN)) || \
	 (defined(BYTE_ORDER) && defined(LITTLE_ENDIAN) && (BYTE_ORDER == LITTLE_ENDIAN)) || \
	 (defined(CPU_X86) || defined(CPU_X86_64)) || \
	 (defined(vax) || defined(MIPSEL) || defined(_MIPSEL)))
#define CPU_LE
#elif ((defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && (__BYTE_ORDER == __BIG_ENDIAN)) || \
	   (defined(BYTE_ORDER) && defined(BIG_ENDIAN) && (BYTE_ORDER == BIG_ENDIAN)) || \
	   (defined(CPU_SPARC) || defined(CPU_PPC) || defined(mc68000) || defined(sel)) || defined(_MIPSEB))
#define CPU_BE
#else
	/* unknown endian! */
#endif


#define U8TO32_BE(p)                                            \
	(((uint32_t)((p)[0]) << 24) | ((uint32_t)((p)[1]) << 16) |  \
	 ((uint32_t)((p)[2]) <<  8) | ((uint32_t)((p)[3])      ))

#define U8TO32_LE(p)                                            \
	(((uint32_t)((p)[0])      ) | ((uint32_t)((p)[1]) <<  8) |  \
	 ((uint32_t)((p)[2]) << 16) | ((uint32_t)((p)[3]) << 24))

#define U32TO8_BE(p, v)                                           \
	(p)[0] = (uint8_t)((v) >> 24); (p)[1] = (uint8_t)((v) >> 16); \
	(p)[2] = (uint8_t)((v) >>  8); (p)[3] = (uint8_t)((v)      );

#define U32TO8_LE(p, v)                                           \
	(p)[0] = (uint8_t)((v)      ); (p)[1] = (uint8_t)((v) >>  8); \
	(p)[2] = (uint8_t)((v) >> 16); (p)[3] = (uint8_t)((v) >> 24);

#define U8TO64_BE(p)                                                  \
	(((uint64_t)U8TO32_BE(p) << 32) | (uint64_t)U8TO32_BE((p) + 4))

#define U8TO64_LE(p)                                                  \
	(((uint64_t)U8TO32_LE(p)) | ((uint64_t)U8TO32_LE((p) + 4) << 32))

#define U64TO8_BE(p, v)                        \
	U32TO8_BE((p),     (uint32_t)((v) >> 32)); \
	U32TO8_BE((p) + 4, (uint32_t)((v)      ));

#define U64TO8_LE(p, v)                        \
	U32TO8_LE((p),     (uint32_t)((v)      )); \
	U32TO8_LE((p) + 4, (uint32_t)((v) >> 32));

#define U32_SWAP(v) {                                             \
	(v) = (((v) << 8) & 0xFF00FF00 ) | (((v) >> 8) & 0xFF00FF );  \
    (v) = ((v) << 16) | ((v) >> 16);                              \
}

#define U64_SWAP(v) {                                                                       \
	(v) = (((v) <<  8) & 0xFF00FF00FF00FF00ull ) | (((v) >>  8) & 0x00FF00FF00FF00FFull );  \
	(v) = (((v) << 16) & 0xFFFF0000FFFF0000ull ) | (((v) >> 16) & 0x0000FFFF0000FFFFull );  \
    (v) = ((v) << 32) | ((v) >> 32);                                                        \
}

static int
scrypt_verify(const uint8_t *x, const uint8_t *y, size_t len) {
	uint32_t differentbits = 0;
	while (len--)
		differentbits |= (*x++ ^ *y++);
	return (1 & ((differentbits - 1) >> 8));
}

void
scrypt_ensure_zero(void *p, size_t len) {
#if ((defined(CPU_X86) || defined(CPU_X86_64)) && defined(COMPILER_MSVC))
		__stosb((unsigned char *)p, 0, len);
#elif (defined(CPU_X86) && defined(COMPILER_GCC))
	__asm__ __volatile__(
		"pushl %%edi;\n"
		"pushl %%ecx;\n"
		"rep stosb;\n"
		"popl %%ecx;\n"
		"popl %%edi;\n"
		:: "a"(0), "D"(p), "c"(len) : "cc", "memory"
	);
#elif (defined(CPU_X86_64) && defined(COMPILER_GCC))
	__asm__ __volatile__(
		"pushq %%rdi;\n"
		"pushq %%rcx;\n"
		"rep stosb;\n"
		"popq %%rcx;\n"
		"popq %%rdi;\n"
		:: "a"(0), "D"(p), "c"(len) : "cc", "memory"
	);
#else
	volatile uint8_t *b = (volatile uint8_t *)p;
	size_t i;
	for (i = 0; i < len; i++)
		b[i] = 0;
#endif
}

#include "scrypt-jane-portable-x86.h"

