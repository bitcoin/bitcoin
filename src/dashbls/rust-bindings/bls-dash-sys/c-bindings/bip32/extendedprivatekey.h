#ifndef BIP32EXTENDEDPRIVATEKEY_H_
#define BIP32EXTENDEDPRIVATEKEY_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "extendedpublickey.h"
#include "chaincode.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef void* BIP32ExtendedPrivateKey;

// ExtendedPrivateKey
BIP32ExtendedPrivateKey BIP32ExtendedPrivateKeyFromBytes(
    const void* data,
    bool* didErr);
BIP32ExtendedPrivateKey BIP32ExtendedPrivateKeyFromSeed(const void* data, const size_t len, bool* didErr);
BIP32ExtendedPrivateKey BIP32ExtendedPrivateKeyPrivateChild(
    const BIP32ExtendedPrivateKey sk,
    const uint32_t index,
    const bool legacy);
BIP32ExtendedPublicKey BIP32ExtendedPrivateKeyPublicChild(
    const BIP32ExtendedPrivateKey sk,
    const uint32_t index);
BIP32ChainCode BIP32ExtendedPrivateKeyGetChainCode(const BIP32ExtendedPrivateKey sk);
void* BIP32ExtendedPrivateKeySerialize(const BIP32ExtendedPrivateKey sk);
bool BIP32ExtendedPrivateKeyIsEqual(
    const BIP32ExtendedPrivateKey sk1,
    const BIP32ExtendedPrivateKey sk2);
void* BIP32ExtendedPrivateKeyGetPrivateKey(const BIP32ExtendedPrivateKey sk);
BIP32ExtendedPublicKey BIP32ExtendedPrivateKeyGetExtendedPublicKey(
    const BIP32ExtendedPrivateKey sk,
    const bool legacy,
    bool* didErr);
void* BIP32ExtendedPrivateKeyGetPublicKey(
    const BIP32ExtendedPrivateKey sk,
    bool* didErr);
void BIP32ExtendedPrivateKeyFree(const BIP32ExtendedPrivateKey sk);

#ifdef __cplusplus
}
#endif
#endif  // BIP32EXTENDEDPRIVATEKEY_H_
