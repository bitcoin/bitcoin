#ifndef _SECP256K1_ECDSA_
#define _SECP256K1_ECDSA_

namespace secp256k1 {

class Signature {
private:
    secp256k1_num_t r,s;

public:
    Signature() {
        secp256k1_num_init(&r);
        secp256k1_num_init(&s);
    }
    ~Signature() {
        secp256k1_num_free(&r);
        secp256k1_num_free(&s);
    }

    bool Parse(const unsigned char *sig, int size);
    bool Serialize(unsigned char *sig, int *size);
    bool RecomputeR(secp256k1_num_t &r2, const GroupElemJac &pubkey, const secp256k1_num_t &message) const;
    bool Verify(const GroupElemJac &pubkey, const secp256k1_num_t &message) const;
    bool Sign(const secp256k1_num_t &seckey, const secp256k1_num_t &message, const secp256k1_num_t &nonce);
    void SetRS(const secp256k1_num_t &rin, const secp256k1_num_t &sin);
    std::string ToString() const;
};

}

#endif
