#if defined(SCRYPT_CHACHA)
#include "scrypt-jane-chacha.h"
#elif defined(SCRYPT_SALSA)
#include "scrypt-jane-salsa.h"
#elif defined(SCRYPT_SALSA64)
#include "scrypt-jane-salsa64.h"
#else
	#define SCRYPT_MIX_BASE "ERROR"
	typedef uint32_t scrypt_mix_word_t;
	#define SCRYPT_WORDTO8_LE U32TO8_LE
	#define SCRYPT_WORD_ENDIAN_SWAP U32_SWAP
	#define SCRYPT_BLOCK_BYTES 64
	#define SCRYPT_BLOCK_WORDS (SCRYPT_BLOCK_BYTES / sizeof(scrypt_mix_word_t))
	#if !defined(SCRYPT_CHOOSE_COMPILETIME)
		static void FASTCALL scrypt_ROMix_error(scrypt_mix_word_t *X/*[chunkWords]*/, scrypt_mix_word_t *Y/*[chunkWords]*/, scrypt_mix_word_t *V/*[chunkWords * N]*/, uint32_t N, uint32_t r) {}
		static scrypt_ROMixfn scrypt_getROMix() { return scrypt_ROMix_error; }
	#else
		static void FASTCALL scrypt_ROMix(scrypt_mix_word_t *X, scrypt_mix_word_t *Y, scrypt_mix_word_t *V, uint32_t N, uint32_t r) {}
	#endif
	static int scrypt_test_mix() { return 0; }
	#error must define a mix function!
#endif

#if !defined(SCRYPT_CHOOSE_COMPILETIME)
#undef SCRYPT_MIX
#define SCRYPT_MIX SCRYPT_MIX_BASE
#endif
