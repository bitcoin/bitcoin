#if defined(SCRYPT_BLAKE512)
#include "scrypt-jane-hash_blake512.h"
#elif defined(SCRYPT_BLAKE256)
#include "scrypt-jane-hash_blake256.h"
#elif defined(SCRYPT_SHA512)
#include "scrypt-jane-hash_sha512.h"
#elif defined(SCRYPT_SHA256)
#include "scrypt-jane-hash_sha256.h"
#elif defined(SCRYPT_SKEIN512)
#include "scrypt-jane-hash_skein512.h"
#elif defined(SCRYPT_KECCAK512) || defined(SCRYPT_KECCAK256)
#include "scrypt-jane-hash_keccak.h"
#else
	#define SCRYPT_HASH "ERROR"
	#define SCRYPT_HASH_BLOCK_SIZE 64
	#define SCRYPT_HASH_DIGEST_SIZE 64
	typedef struct scrypt_hash_state_t { size_t dummy; } scrypt_hash_state;
	typedef uint8_t scrypt_hash_digest[SCRYPT_HASH_DIGEST_SIZE];
	static void scrypt_hash_init(scrypt_hash_state *S) {}
	static void scrypt_hash_update(scrypt_hash_state *S, const uint8_t *in, size_t inlen) {}
	static void scrypt_hash_finish(scrypt_hash_state *S, uint8_t *hash) {}
	static const uint8_t scrypt_test_hash_expected[SCRYPT_HASH_DIGEST_SIZE] = {0};
	#error must define a hash function!
#endif

#include "scrypt-jane-pbkdf2.h"

#define SCRYPT_TEST_HASH_LEN 257 /* (2 * largest block size) + 1 */

static int
scrypt_test_hash() {
	scrypt_hash_state st;
	scrypt_hash_digest hash, final;
	uint8_t msg[SCRYPT_TEST_HASH_LEN];
	size_t i;

	for (i = 0; i < SCRYPT_TEST_HASH_LEN; i++)
		msg[i] = (uint8_t)i;

	scrypt_hash_init(&st);
	for (i = 0; i < SCRYPT_TEST_HASH_LEN + 1; i++) {
		scrypt_hash(hash, msg, i);
		scrypt_hash_update(&st, hash, sizeof(hash));
	}
	scrypt_hash_finish(&st, final);
	return scrypt_verify(final, scrypt_test_hash_expected, SCRYPT_HASH_DIGEST_SIZE);
}

