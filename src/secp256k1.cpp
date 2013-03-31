#include "num.cpp"
#include "field.cpp"
#include "group.cpp"
#include "ecmult.cpp"
#include "ecdsa.cpp"

namespace secp256k1 {

int VerifyECDSA(const unsigned char *msg, int msglen, const unsigned char *sig, int siglen, const unsigned char *pubkey, int pubkeylen) {
    int ret = -3;
    secp256k1_num_t m; 
    secp256k1_num_init(&m);
    Signature s;
    GroupElemJac q;
    secp256k1_num_set_bin(&m, msg, msglen);
    if (!ParsePubKey(q, pubkey, pubkeylen)) {
        ret = -1;
        goto end;
    }
    if (!s.Parse(sig, siglen)) {
        fprintf(stderr, "Can't parse signature: ");
        for (int i=0; i<siglen; i++) fprintf(stderr,"%02x", sig[i]);
        fprintf(stderr, "\n");
        ret = -2;
        goto end;
    }
    if (!s.Verify(q, m)) {
        ret = 0;
        goto end;
    }
    ret = 1;
end:
    secp256k1_num_free(&m);
    return ret;
}


}
