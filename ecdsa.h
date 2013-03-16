#ifndef _SECP256K1_ECDSA_
#define _SECP256K1_ECDSA_

namespace secp256k1 {

class Signature {
private:
    Number r,s;

public:
    bool Parse(const unsigned char *sig, int size);
    bool RecomputeR(Number &r2, const GroupElemJac &pubkey, const Number &message);
    bool Verify(const GroupElemJac &pubkey, const Number &message);
    void SetRS(const Number &rin, const Number &sin);
    std::string ToString() const;
};

}

#endif
