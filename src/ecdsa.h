#ifndef _SECP256K1_ECDSA_
#define _SECP256K1_ECDSA_

#include "num.h"

typedef struct {
    secp256k1_num_t r, s;
} secp256k1_ecdsa_sig_t;

void static secp256k1_ecdsa_sig_init(secp256k1_ecdsa_sig_t *r);
void static secp256k1_ecdsa_sig_free(secp256k1_ecdsa_sig_t *r);

int static secp256k1_ecdsa_pubkey_parse(secp256k1_gej_t *elem, const unsigned char *pub, int size);
int static secp256k1_ecdsa_sig_parse(secp256k1_ecdsa_sig_t *r, const unsigned char *sig, int size);
int static secp256k1_ecdsa_sig_serialize(unsigned char *sig, int *size, const secp256k1_ecdsa_sig_t *a);
int static secp256k1_ecdsa_sig_sign(secp256k1_ecdsa_sig_t *sig, const secp256k1_num_t *seckey, const secp256k1_num_t *message, const secp256k1_num_t *nonce);
void static secp256k1_ecdsa_sig_set_rs(secp256k1_ecdsa_sig_t *sig, const secp256k1_num_t *r, const secp256k1_num_t *s);

#endif
