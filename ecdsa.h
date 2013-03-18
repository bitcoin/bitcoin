#ifndef _SECP256K1_ECDSA_
#define _SECP256K1_ECDSA_

namespace secp256k1 {

class Signature {
private:
    Number r,s;

public:
    bool Parse(const unsigned char *sig, int size);
    bool Serialize(unsigned char *sig, int *size);
    bool RecomputeR(Number &r2, const GroupElemJac &pubkey, const Number &message) const;
    bool Verify(const GroupElemJac &pubkey, const Number &message) const;
    bool Sign(const Number &seckey, const Number &message, const Number &nonce);
    void SetRS(const Number &rin, const Number &sin);
    std::string ToString() const;
};

}

#endif
