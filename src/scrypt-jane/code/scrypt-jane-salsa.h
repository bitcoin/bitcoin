#define SCRYPT_MIX_BASE "Salsa20/8"

typedef uint32_t scrypt_mix_word_t;

#define SCRYPT_WORDTO8_LE U32TO8_LE
#define SCRYPT_WORD_ENDIAN_SWAP U32_SWAP

#define SCRYPT_BLOCK_BYTES 64
#define SCRYPT_BLOCK_WORDS (SCRYPT_BLOCK_BYTES / sizeof(scrypt_mix_word_t))

/* must have these here in case block bytes is ever != 64 */
#include "scrypt-jane-romix-basic.h"

#include "scrypt-jane-mix_salsa-avx.h"
#include "scrypt-jane-mix_salsa-sse2.h"
#include "scrypt-jane-mix_salsa.h"

#if defined(SCRYPT_SALSA_AVX)
	#define SCRYPT_CHUNKMIX_FN scrypt_ChunkMix_avx
	#define SCRYPT_ROMIX_FN scrypt_ROMix_avx
	#define SCRYPT_ROMIX_TANGLE_FN salsa_core_tangle_sse2
	#define SCRYPT_ROMIX_UNTANGLE_FN salsa_core_tangle_sse2
	#include "scrypt-jane-romix-template.h"
#endif

#if defined(SCRYPT_SALSA_SSE2)
	#define SCRYPT_CHUNKMIX_FN scrypt_ChunkMix_sse2
	#define SCRYPT_ROMIX_FN scrypt_ROMix_sse2
	#define SCRYPT_MIX_FN salsa_core_sse2
	#define SCRYPT_ROMIX_TANGLE_FN salsa_core_tangle_sse2
	#define SCRYPT_ROMIX_UNTANGLE_FN salsa_core_tangle_sse2
	#include "scrypt-jane-romix-template.h"
#endif

/* cpu agnostic */
#define SCRYPT_ROMIX_FN scrypt_ROMix_basic
#define SCRYPT_MIX_FN salsa_core_basic
#define SCRYPT_ROMIX_TANGLE_FN scrypt_romix_convert_endian
#define SCRYPT_ROMIX_UNTANGLE_FN scrypt_romix_convert_endian
#include "scrypt-jane-romix-template.h"

#if !defined(SCRYPT_CHOOSE_COMPILETIME)
static scrypt_ROMixfn
scrypt_getROMix() {
	size_t cpuflags = detect_cpu();

#if defined(SCRYPT_SALSA_AVX)
	if (cpuflags & cpu_avx)
		return scrypt_ROMix_avx;
	else
#endif

#if defined(SCRYPT_SALSA_SSE2)
	if (cpuflags & cpu_sse2)
		return scrypt_ROMix_sse2;
	else
#endif

	return scrypt_ROMix_basic;
}
#endif


#if defined(SCRYPT_TEST_SPEED)
static size_t
available_implementations() {
	size_t flags = 0;

#if defined(SCRYPT_SALSA_AVX)
		flags |= cpu_avx;
#endif

#if defined(SCRYPT_SALSA_SSE2)
		flags |= cpu_sse2;
#endif

	return flags;
}
#endif


static int
scrypt_test_mix() {
	static const uint8_t expected[16] = {
		0x41,0x1f,0x2e,0xa3,0xab,0xa3,0x1a,0x34,0x87,0x1d,0x8a,0x1c,0x76,0xa0,0x27,0x66,
	};

	int ret = 1;
	size_t cpuflags = detect_cpu();

#if defined(SCRYPT_SALSA_AVX)
	if (cpuflags & cpu_avx)
		ret &= scrypt_test_mix_instance(scrypt_ChunkMix_avx, salsa_core_tangle_sse2, salsa_core_tangle_sse2, expected);
#endif

#if defined(SCRYPT_SALSA_SSE2)
	if (cpuflags & cpu_sse2)
		ret &= scrypt_test_mix_instance(scrypt_ChunkMix_sse2, salsa_core_tangle_sse2, salsa_core_tangle_sse2, expected);
#endif

#if defined(SCRYPT_SALSA_BASIC)
	ret &= scrypt_test_mix_instance(scrypt_ChunkMix_basic, scrypt_romix_convert_endian, scrypt_romix_convert_endian, expected);
#endif

	return ret;
}
