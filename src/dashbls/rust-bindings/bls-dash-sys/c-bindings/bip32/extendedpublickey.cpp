#include "extendedpublickey.h"

#include <vector>

#include "../blschia.h"
#include "../error.h"
#include "bls.hpp"

BIP32ExtendedPublicKey BIP32ExtendedPublicKeyFromBytes(
    const void* data,
    const bool legacy,
    bool* didErr)
{
    bls::ExtendedPublicKey* el = nullptr;
    try {
        el = new bls::ExtendedPublicKey(bls::ExtendedPublicKey::FromBytes(
            bls::Bytes((uint8_t*)(data), bls::ExtendedPublicKey::SIZE),
            legacy));
    } catch (const std::exception& ex) {
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;
    return el;
}

BIP32ExtendedPublicKey BIP32ExtendedPublicKeyPublicChild(
    const BIP32ExtendedPublicKey pk,
    const uint32_t index,
    const bool legacy)
{
    const bls::ExtendedPublicKey* pkPtr = (bls::ExtendedPublicKey*)pk;
    return new bls::ExtendedPublicKey(pkPtr->PublicChild(index, legacy));
}

BIP32ChainCode BIP32ExtendedPublicKeyGetChainCode(const BIP32ExtendedPublicKey pk)
{
    const bls::ExtendedPublicKey* pkPtr = (bls::ExtendedPublicKey*)pk;
    return new bls::ChainCode(pkPtr->GetChainCode());
}

void* BIP32ExtendedPublicKeyGetPublicKey(const BIP32ExtendedPublicKey pk) {
    bls::ExtendedPublicKey* pkPtr = (bls::ExtendedPublicKey*)pk;
    return new bls::G1Element(pkPtr->GetPublicKey());
}

void* BIP32ExtendedPublicKeySerialize(
    const BIP32ExtendedPublicKey pk,
    const bool legacy)
{
    const bls::ExtendedPublicKey* pkPtr = (bls::ExtendedPublicKey*)pk;
    const std::vector<uint8_t> serialized = pkPtr->Serialize(legacy);
    uint8_t* buffer = (uint8_t*)malloc(bls::ExtendedPublicKey::SIZE);
    memcpy(buffer, serialized.data(), bls::ExtendedPublicKey::SIZE);
    return (void*)buffer;
}

bool BIP32ExtendedPublicKeyIsEqual(
    const BIP32ExtendedPublicKey pk1,
    const BIP32ExtendedPublicKey pk2)
{
    const bls::ExtendedPublicKey* pk1Ptr = (bls::ExtendedPublicKey*)pk1;
    const bls::ExtendedPublicKey* pk2Ptr = (bls::ExtendedPublicKey*)pk2;
    return *pk1Ptr == *pk2Ptr;
}

void BIP32ExtendedPublicKeyFree(const BIP32ExtendedPublicKey pk)
{
    const bls::ExtendedPublicKey* pkPtr = (bls::ExtendedPublicKey*)pk;
    delete pkPtr;
}
