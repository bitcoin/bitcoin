#include "num.cpp"
#include "field.cpp"
#include "group.cpp"
#include "ecmult.cpp"
#include "ecdsa.cpp"

namespace secp256k1 {

int VerifyECDSA(const unsigned char *msg, int msglen, const unsigned char *sig, int siglen, const unsigned char *pubkey, int pubkeylen) {
    Number m; 
    Signature s;
    GroupElemJac q;
    m.SetBytes(msg, msglen);
    if (!ParsePubKey(q, pubkey, pubkeylen))
        return -1;
    if (!s.Parse(sig, siglen)) {
        fprintf(stderr, "Can't parse signature: ");
        for (int i=0; i<siglen; i++) fprintf(stderr,"%02x", sig[i]);
        fprintf(stderr, "\n");
        return -2;
    }
//    fprintf(stderr, "Verifying ECDSA: msg=%s pubkey=%s sig=%s\n", m.ToString().c_str(), q.ToString().c_str(), s.ToString().c_str());
    if (!s.Verify(q, m))
        return 0;
    return 1;
}

}