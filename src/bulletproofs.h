#ifndef BITCOIN_BULLETPROOFS_H
#define BITCOIN_BULLETPROOFS_H

#include <vector>

struct CBulletproof {
    std::vector<unsigned char> proof;
};

inline bool VerifyBulletproof(const CBulletproof& proof)
{
#ifdef ENABLE_BULLETPROOFS
    (void)proof; // TODO: call into libsecp256k1-zkp verification
    return true;
#else
    (void)proof;
    return false;
#endif
}

#endif // BITCOIN_BULLETPROOFS_H
