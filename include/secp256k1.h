#ifndef _SECP256K1_
#define _SECP256K1_

namespace secp256k1 {
int VerifyECDSA(const unsigned char *msg, int msglen, const unsigned char *sig, int siglen, const unsigned char *pubkey, int pubkeylen);
}

#endif
