#ifndef BIP32EXTENDEDPUBLICKEY_H_
#define BIP32EXTENDEDPUBLICKEY_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "chaincode.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* BIP32ExtendedPublicKey;

// ExtendedPublicKey
BIP32ExtendedPublicKey BIP32ExtendedPublicKeyFromBytes(
    const void* data,
    const bool legacy,
    bool* didErr);
BIP32ExtendedPublicKey BIP32ExtendedPublicKeyPublicChild(
    const BIP32ExtendedPublicKey pk,
    const uint32_t index,
    const bool legacy);
BIP32ChainCode BIP32ExtendedPublicKeyGetChainCode(const BIP32ExtendedPublicKey pk);
void* BIP32ExtendedPublicKeyGetPublicKey(const BIP32ExtendedPublicKey pk);
void* BIP32ExtendedPublicKeySerialize(
    const BIP32ExtendedPublicKey pk,
    const bool legacy);
bool BIP32ExtendedPublicKeyIsEqual(
    const BIP32ExtendedPublicKey pk1,
    const BIP32ExtendedPublicKey pk2);
void BIP32ExtendedPublicKeyFree(const BIP32ExtendedPublicKey pk);

#ifdef __cplusplus
}
#endif
#endif  // BIP32EXTENDEDPUBLICKEY_H_
