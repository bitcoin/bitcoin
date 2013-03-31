#ifndef _SECP256K1_NUM_
#define _SECP256K1_NUM_

#if defined(USE_NUM_GMP)
#include "num_gmp.h"
#elif defined(USE_NUM_OPENSSL)
#include "num_openssl.h"
#else
#error "Please select num implementation"
#endif

extern "C" {

void static secp256k1_num_start(void);
void static secp256k1_num_init(secp256k1_num_t *r);
void static secp256k1_num_free(secp256k1_num_t *r);
void static secp256k1_num_copy(secp256k1_num_t *r, const secp256k1_num_t *a);
void static secp256k1_num_get_bin(unsigned char *r, unsigned int rlen, const secp256k1_num_t *a);
void static secp256k1_num_set_bin(secp256k1_num_t *r, const unsigned char *a, unsigned int alen);
void static secp256k1_num_set_int(secp256k1_num_t *r, int a);
void static secp256k1_num_mod_inverse(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *m);
void static secp256k1_num_mod_mul(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *b, const secp256k1_num_t *m);
int  static secp256k1_num_cmp(const secp256k1_num_t *a, const secp256k1_num_t *b);
void static secp256k1_num_add(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *b);
void static secp256k1_num_sub(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *b);
void static secp256k1_num_mul(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *b);
void static secp256k1_num_div(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *b);
void static secp256k1_num_mod(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *b);
int  static secp256k1_num_bits(const secp256k1_num_t *a);
int  static secp256k1_num_shift(secp256k1_num_t *r, int bits);
int  static secp256k1_num_is_zero(const secp256k1_num_t *a);
int  static secp256k1_num_is_odd(const secp256k1_num_t *a);
int  static secp256k1_num_is_neg(const secp256k1_num_t *a);
int  static secp256k1_num_get_bit(const secp256k1_num_t *a, int pos);
void static secp256k1_num_inc(secp256k1_num_t *r);
void static secp256k1_num_set_hex(secp256k1_num_t *r, const char *a, int alen);
void static secp256k1_num_get_hex(char *r, int *rlen, const secp256k1_num_t *a);
void static secp256k1_num_split(secp256k1_num_t *rl, secp256k1_num_t *rh, const secp256k1_num_t *a, int bits);
void static secp256k1_num_negate(secp256k1_num_t *r);
void static secp256k1_num_set_rand(secp256k1_num_t *r, const secp256k1_num_t *a);

}

#endif
