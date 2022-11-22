#include <stdio.h>
#include <assert.h>

#include "relic.h"
#include "relic_test.h"

static int paillier(void) {
	int code = RLC_ERR;
	bn_t a, b, c, d, s, pub, prv;
	int result;

	bn_null(a);
	bn_null(b);
	bn_null(c);
	bn_null(d);

	bn_null(pub);
	bn_null(prv);

	RLC_TRY {
		bn_new(a);
		bn_new(b);
		bn_new(c);
		bn_new(d);

		bn_new(pub);
		bn_new(prv);

		/* Generate 2048-bit public and private keys (both integers). */
		result = cp_ghpe_gen(pub, prv, 2048);
		assert(result == RLC_OK);

		printf("Public key:\n");
		bn_print(pub);
		printf("Private key:\n");
		bn_print(prv);

		for (int s = 1; s <= 2; s++) {
			util_print("Testing generalized paillier for (s = %d)\n", s);
			/* Generate plaintext smaller than n^s. */
			bn_rand(a, RLC_POS, s * (bn_bits(pub) - 1));
			/* Encrypt, decrypt and check if the result is the same. */
			assert(cp_ghpe_enc(c, a, pub, s) == RLC_OK);
			assert(cp_ghpe_dec(b, c, pub, prv, s) == RLC_OK);
			assert(bn_cmp(a, b) == RLC_EQ);

			/* Generate new plaintexts smaller than n^s. */
			bn_rand(a, RLC_POS, s * (bn_bits(pub) - 1));
			bn_rand(b, RLC_POS, s * (bn_bits(pub) - 1));
			/* Encrypt both plaintexts using the same public key. */
			assert(cp_ghpe_enc(c, a, pub, s) == RLC_OK);
			assert(cp_ghpe_enc(d, b, pub, s) == RLC_OK);

			/* Now compute c = (b * c) mod n^(s + 1). */
			bn_mul(c, c, d);
			bn_sqr(d, pub);
			if (s == 2) {
				bn_mul(d, d, pub);
			}
			bn_mod(c, c, d);
			/* Decrypt and check if result is (a + b) mod n^s. */
			assert(cp_ghpe_dec(c, c, pub, prv, s) == RLC_OK);
			bn_add(a, a, b);
			bn_copy(d, pub);
			if (s == 2) {
				bn_mul(d, d, pub);
			}
			bn_mod(a, a, d);
			assert(bn_cmp(a, c) == RLC_EQ);
		}
	}
	RLC_CATCH_ANY {
		RLC_ERROR(end);
	}
	code = RLC_OK;

  end:
	bn_free(a);
	bn_free(b);
	bn_free(c);
	bn_free(d);

	bn_free(prv);
	bn_free(pub);
	return code;
}

int main(int argc, char *argv[]) {
	core_init();
	paillier();
	core_clean();
}
