#if defined(CPU_X86) && (defined(COMPILER_MSVC) || defined(COMPILER_GCC))
	#define X86ASM
	/* gcc 2.95 royally screws up stack alignments on variables */
	#if (defined(COMPILER_MSVC6PP_AND_LATER) || (defined(COMPILER_GCC) && (COMPILER_GCC >= 30000)))
		#define X86ASM_SSE
		#define X86ASM_SSE2
	#endif
	#if ((defined(COMPILER_MSVC) && (COMPILER_MSVC >= 1400)) || (defined(COMPILER_GCC) && (COMPILER_GCC >= 40102)))
		#define X86ASM_SSSE3
	#endif
	#if ((defined(COMPILER_GCC) && (COMPILER_GCC >= 40400)))
		#define X86ASM_AVX
	#endif
#endif

#if defined(CPU_X86_64) && defined(COMPILER_GCC)
	#define X86_64ASM
	#define X86_64ASM_SSE2
	#if (COMPILER_GCC >= 40102)
		#define X86_64ASM_SSSE3
	#endif
	#if (COMPILER_GCC >= 40400)
		#define X86_64ASM_AVX
	#endif
#endif

#if defined(COMPILER_MSVC)
	#define X86_INTRINSIC
	#if defined(CPU_X86_64) || defined(X86ASM_SSE)
		#define X86_INTRINSIC_SSE
	#endif
	#if defined(CPU_X86_64) || defined(X86ASM_SSE2)
		#define X86_INTRINSIC_SSE2
	#endif
	#if (COMPILER_MSVC >= 1400)
		#define X86_INTRINSIC_SSSE3
	#endif
#endif

#if defined(COMPILER_MSVC) && defined(CPU_X86_64)
	#define X86_64USE_INTRINSIC
#endif

#if defined(COMPILER_MSVC) && defined(CPU_X86_64)
	#define X86_64USE_INTRINSIC
#endif

#if defined(COMPILER_GCC) && defined(CPU_X86_FORCE_INTRINSICS)
	#define X86_INTRINSIC
	#if defined(__SSE__)
		#define X86_INTRINSIC_SSE
	#endif
	#if defined(__SSE2__)
		#define X86_INTRINSIC_SSE2
	#endif
	#if defined(__SSSE3__)
		#define X86_INTRINSIC_SSSE3
	#endif
	#if defined(__AVX__)
		#define X86_INTRINSIC_AVX
	#endif
#endif

/* only use simd on windows (or SSE2 on gcc)! */
#if defined(CPU_X86_FORCE_INTRINSICS) || defined(X86_INTRINSIC)
	#if defined(X86_INTRINSIC_SSE)
		#define X86_INTRINSIC
		#include <mmintrin.h>
		#include <xmmintrin.h>
		typedef __m64 qmm;
		typedef __m128 xmm;
		typedef __m128d xmmd;
	#endif
	#if defined(X86_INTRINSIC_SSE2)
		#define X86_INTRINSIC_SSE2
		#include <emmintrin.h>
		typedef __m128i xmmi;
	#endif
	#if defined(X86_INTRINSIC_SSSE3)
		#define X86_INTRINSIC_SSSE3
		#include <tmmintrin.h>
	#endif
#endif


#if defined(X86_INTRINSIC_SSE2)
	typedef union packedelem8_t {
		uint8_t u[16];
		xmmi v;	
	} packedelem8;

	typedef union packedelem32_t {
		uint32_t u[4];
		xmmi v;	
	} packedelem32;

	typedef union packedelem64_t {
		uint64_t u[2];
		xmmi v;	
	} packedelem64;
#else
	typedef union packedelem8_t {
		uint8_t u[16];
		uint32_t dw[4];		
	} packedelem8;

	typedef union packedelem32_t {
		uint32_t u[4];
		uint8_t b[16];
	} packedelem32;

	typedef union packedelem64_t {
		uint64_t u[2];
		uint8_t b[16];
	} packedelem64;
#endif

#if defined(X86_INTRINSIC_SSSE3) || defined(X86ASM_SSSE3) || defined(X86_64ASM_SSSE3)
	const packedelem8 MM16 ssse3_rotr16_64bit      = {{2,3,4,5,6,7,0,1,10,11,12,13,14,15,8,9}};
	const packedelem8 MM16 ssse3_rotl16_32bit      = {{2,3,0,1,6,7,4,5,10,11,8,9,14,15,12,13}};
	const packedelem8 MM16 ssse3_rotl8_32bit       = {{3,0,1,2,7,4,5,6,11,8,9,10,15,12,13,14}};
	const packedelem8 MM16 ssse3_endian_swap_64bit = {{7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8}};
#endif

/*
	x86 inline asm for gcc/msvc. usage:

	asm_naked_fn_proto(return_type, name) (type parm1, type parm2..)
	asm_naked_fn(name)
		a1(..)
		a2(.., ..)
		a3(.., .., ..)
		a1(ret)
	asm_naked_fn_end(name)
*/

#if defined(X86ASM) || defined(X86_64ASM)

#if defined(COMPILER_MSVC)
	#pragma warning(disable : 4731) /* frame pointer modified by inline assembly */
	#define a1(x) __asm {x}
	#define a2(x, y) __asm {x, y}
	#define a3(x, y, z) __asm {x, y, z}
	#define a4(x, y, z, w) __asm {x, y, z, w}
	#define al(x) __asm {label##x:}
	#define aj(x, y, z) __asm {x label##y}
	#define asm_align8 a1(ALIGN 8)
	#define asm_align16 a1(ALIGN 16)

	#define asm_naked_fn_proto(type, fn) static NAKED type STDCALL fn
	#define asm_naked_fn(fn) {
	#define asm_naked_fn_end(fn) }
#elif defined(COMPILER_GCC)
	#define GNU_AS1(x) #x ";\n"
	#define GNU_AS2(x, y) #x ", " #y ";\n"
	#define GNU_AS3(x, y, z) #x ", " #y ", " #z ";\n"
	#define GNU_AS4(x, y, z, w) #x ", " #y ", " #z ", " #w ";\n"
	#define GNU_ASL(x) "\n" #x ":\n"
	#define GNU_ASJ(x, y, z) #x " " #y #z ";"

	#define a1(x) GNU_AS1(x)
	#define a2(x, y) GNU_AS2(x, y)
	#define a3(x, y, z) GNU_AS3(x, y, z)
	#define a4(x, y, z, w) GNU_AS4(x, y, z, w)
	#define al(x) GNU_ASL(x)
	#define aj(x, y, z) GNU_ASJ(x, y, z)
	#define asm_align8 a1(.align 8)
	#define asm_align16 a1(.align 16)

	#define asm_naked_fn_proto(type, fn) extern type STDCALL fn
	#define asm_naked_fn(fn) ; __asm__ (".intel_syntax noprefix;\n.text\n" asm_align16 GNU_ASL(fn)
	#define asm_naked_fn_end(fn) ".att_syntax prefix;\n.type  " #fn ",@function\n.size " #fn ",.-" #fn "\n" );
	#define asm_gcc() __asm__ __volatile__(".intel_syntax noprefix;\n"
	#define asm_gcc_parms() ".att_syntax prefix;"
	#define asm_gcc_trashed() __asm__ __volatile__("" :::
	#define asm_gcc_end() );
#else
	need x86 asm
#endif

#endif /* X86ASM || X86_64ASM */


#if defined(CPU_X86) || defined(CPU_X86_64)

typedef enum cpu_flags_x86_t {
	cpu_mmx = 1 << 0,
	cpu_sse = 1 << 1,
	cpu_sse2 = 1 << 2,
	cpu_sse3 = 1 << 3,
	cpu_ssse3 = 1 << 4,
	cpu_sse4_1 = 1 << 5,
	cpu_sse4_2 = 1 << 6,
	cpu_avx = 1 << 7
} cpu_flags_x86;

typedef enum cpu_vendors_x86_t {
	cpu_nobody,
	cpu_intel,
	cpu_amd
} cpu_vendors_x86;

typedef struct x86_regs_t {
	uint32_t eax, ebx, ecx, edx;
} x86_regs;

#if defined(X86ASM)
asm_naked_fn_proto(int, has_cpuid)(void)
asm_naked_fn(has_cpuid)
	a1(pushfd)
	a1(pop eax)
	a2(mov ecx, eax)
	a2(xor eax, 0x200000)
	a1(push eax)
	a1(popfd)
	a1(pushfd)
	a1(pop eax)
	a2(xor eax, ecx)
	a2(shr eax, 21)
	a2(and eax, 1)
	a1(push ecx)
	a1(popfd)
	a1(ret)
asm_naked_fn_end(has_cpuid)
#endif /* X86ASM */


static void NOINLINE
get_cpuid(x86_regs *regs, uint32_t flags) {
#if defined(COMPILER_MSVC)
	__cpuid((int *)regs, (int)flags);
#else
	#if defined(CPU_X86_64)
		#define cpuid_bx rbx
	#else
		#define cpuid_bx ebx
	#endif

	asm_gcc()
		a1(push cpuid_bx)
		a1(cpuid)
		a2(mov [%1 + 0], eax)
		a2(mov [%1 + 4], ebx)
		a2(mov [%1 + 8], ecx)
		a2(mov [%1 + 12], edx)
		a1(pop cpuid_bx)
		asm_gcc_parms() : "+a"(flags) : "S"(regs)  : "%ecx", "%edx", "cc"
	asm_gcc_end()
#endif
}

#if defined(X86ASM_AVX) || defined(X86_64ASM_AVX)
static uint64_t NOINLINE
get_xgetbv(uint32_t flags) {
#if defined(COMPILER_MSVC)
	return _xgetbv(flags);
#else
	uint32_t lo, hi;
	asm_gcc()
		a1(xgetbv)
		asm_gcc_parms() : "+c"(flags), "=a" (lo), "=d" (hi)
	asm_gcc_end()
	return ((uint64_t)lo | ((uint64_t)hi << 32));
#endif
}
#endif // AVX support

#if defined(SCRYPT_TEST_SPEED)
size_t cpu_detect_mask = (size_t)-1;
#endif

static size_t
detect_cpu(void) {
	union { uint8_t s[12]; uint32_t i[3]; } vendor_string;
	cpu_vendors_x86 vendor = cpu_nobody;
	x86_regs regs;
	uint32_t max_level;
	size_t cpu_flags = 0;
#if defined(X86ASM_AVX) || defined(X86_64ASM_AVX)
	uint64_t xgetbv_flags;
#endif

#if defined(CPU_X86)
	if (!has_cpuid())
		return cpu_flags;
#endif

	get_cpuid(&regs, 0);
	max_level = regs.eax;
	vendor_string.i[0] = regs.ebx;
	vendor_string.i[1] = regs.edx;
	vendor_string.i[2] = regs.ecx;

	if (scrypt_verify(vendor_string.s, (const uint8_t *)"GenuineIntel", 12))
		vendor = cpu_intel;
	else if (scrypt_verify(vendor_string.s, (const uint8_t *)"AuthenticAMD", 12))
		vendor = cpu_amd;
	
	if (max_level & 0x00000500) {
		/* "Intel P5 pre-B0" */
		cpu_flags |= cpu_mmx;
		return cpu_flags;
	}

	if (max_level < 1)
		return cpu_flags;

	get_cpuid(&regs, 1);
#if defined(X86ASM_AVX) || defined(X86_64ASM_AVX)
	/* xsave/xrestore */
	if (regs.ecx & (1 << 27)) {
		xgetbv_flags = get_xgetbv(0);
		if ((regs.ecx & (1 << 28)) && (xgetbv_flags & 0x6)) cpu_flags |= cpu_avx;
	}
#endif
	if (regs.ecx & (1 << 20)) cpu_flags |= cpu_sse4_2;
	if (regs.ecx & (1 << 19)) cpu_flags |= cpu_sse4_2;
	if (regs.ecx & (1 <<  9)) cpu_flags |= cpu_ssse3;
	if (regs.ecx & (1      )) cpu_flags |= cpu_sse3;
	if (regs.edx & (1 << 26)) cpu_flags |= cpu_sse2;
	if (regs.edx & (1 << 25)) cpu_flags |= cpu_sse;
	if (regs.edx & (1 << 23)) cpu_flags |= cpu_mmx;
	
#if defined(SCRYPT_TEST_SPEED)
	cpu_flags &= cpu_detect_mask;
#endif

	return cpu_flags;
}

#if defined(SCRYPT_TEST_SPEED)
static const char *
get_top_cpuflag_desc(size_t flag) {
	if (flag & cpu_avx) return "AVX";
	else if (flag & cpu_sse4_2) return "SSE4.2";
	else if (flag & cpu_sse4_1) return "SSE4.1";
	else if (flag & cpu_ssse3) return "SSSE3";
	else if (flag & cpu_sse2) return "SSE2";
	else if (flag & cpu_sse) return "SSE";
	else if (flag & cpu_mmx) return "MMX";
	else return "Basic";
}
#endif

/* enable the highest system-wide option */
#if defined(SCRYPT_CHOOSE_COMPILETIME)
	#if !defined(__AVX__)
		#undef X86_64ASM_AVX
		#undef X86ASM_AVX
		#undef X86_INTRINSIC_AVX
	#endif
	#if !defined(__SSSE3__)
		#undef X86_64ASM_SSSE3
		#undef X86ASM_SSSE3
		#undef X86_INTRINSIC_SSSE3
	#endif
	#if !defined(__SSE2__)
		#undef X86_64ASM_SSE2
		#undef X86ASM_SSE2
		#undef X86_INTRINSIC_SSE2
	#endif
#endif

#endif /* defined(CPU_X86) || defined(CPU_X86_64) */