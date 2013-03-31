#ifndef _SECP256K1_
#define _SECP256K1_

extern "C" {
void secp256k1_start(void);
void secp256k1_stop(void);
int secp256k1_ecdsa_verify(const unsigned char *msg, int msglen, const unsigned char *sig, int siglen, const unsigned char *pubkey, int pubkeylen);
}

#endif
