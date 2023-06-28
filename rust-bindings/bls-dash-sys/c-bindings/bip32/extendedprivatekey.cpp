#include "extendedprivatekey.h"

#include <vector>

#include "../blschia.h"
#include "../error.h"
#include "bls.hpp"

BIP32ExtendedPrivateKey BIP32ExtendedPrivateKeyFromBytes(const void* data, bool* didErr)
{
    bls::ExtendedPrivateKey* el = nullptr;
    try {
        el = new bls::ExtendedPrivateKey(bls::ExtendedPrivateKey::FromBytes(
            bls::Bytes((uint8_t*)(data), bls::ExtendedPrivateKey::SIZE)));
    } catch (const std::exception& ex) {
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;
    return el;
}

BIP32ExtendedPrivateKey BIP32ExtendedPrivateKeyFromSeed(const void* data, const size_t len, bool* didErr)
{
    bls::ExtendedPrivateKey* el = nullptr;
    try {
        el = new bls::ExtendedPrivateKey(bls::ExtendedPrivateKey::FromSeed(
            bls::Bytes((uint8_t*)(data), len)));
    } catch (const std::exception& ex) {
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;
    return el;
}

BIP32ExtendedPrivateKey BIP32ExtendedPrivateKeyPrivateChild(
    const BIP32ExtendedPrivateKey sk,
    const uint32_t index,
    const bool legacy)
{
    const bls::ExtendedPrivateKey* skPtr = (bls::ExtendedPrivateKey*)sk;
    return new bls::ExtendedPrivateKey(skPtr->PrivateChild(index, legacy));
}

BIP32ExtendedPublicKey BIP32ExtendedPrivateKeyPublicChild(
    const BIP32ExtendedPrivateKey sk,
    const uint32_t index)
{
    const bls::ExtendedPrivateKey* skPtr = (bls::ExtendedPrivateKey*)sk;
    return new bls::ExtendedPublicKey(skPtr->PublicChild(index));
}

BIP32ChainCode BIP32ExtendedPrivateKeyGetChainCode(const BIP32ExtendedPrivateKey sk)
{
    const bls::ExtendedPrivateKey* skPtr = (bls::ExtendedPrivateKey*)sk;
    return new bls::ChainCode(skPtr->GetChainCode());
}

void* BIP32ExtendedPrivateKeySerialize(const BIP32ExtendedPrivateKey sk)
{
    const bls::ExtendedPrivateKey* skPtr = (bls::ExtendedPrivateKey*)sk;
    uint8_t* buffer =
        bls::Util::SecAlloc<uint8_t>(bls::ExtendedPrivateKey::SIZE);
    skPtr->Serialize(buffer);

    return (void*)buffer;
}

bool BIP32ExtendedPrivateKeyIsEqual(
    const BIP32ExtendedPrivateKey sk1,
    const BIP32ExtendedPrivateKey sk2)
{
    const bls::ExtendedPrivateKey* sk1Ptr = (bls::ExtendedPrivateKey*)sk1;
    const bls::ExtendedPrivateKey* sk2Ptr = (bls::ExtendedPrivateKey*)sk2;
    return *sk1Ptr == *sk2Ptr;
}

void* BIP32ExtendedPrivateKeyGetPrivateKey(const BIP32ExtendedPrivateKey sk)
{
    bls::ExtendedPrivateKey* skPtr = (bls::ExtendedPrivateKey*)sk;
    return new bls::PrivateKey(skPtr->GetPrivateKey());
}

void* BIP32ExtendedPrivateKeyGetPublicKey(
    const BIP32ExtendedPrivateKey sk,
    bool* didErr)
{
    bls::ExtendedPrivateKey* skPtr = (bls::ExtendedPrivateKey*)sk;
    bls::G1Element* el = nullptr;
    try {
        el = new bls::G1Element(skPtr->GetPublicKey());
        *didErr = false;
    } catch (const std::exception& ex) {
        // set err
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    return el;
}

BIP32ExtendedPublicKey BIP32ExtendedPrivateKeyGetExtendedPublicKey(
    const BIP32ExtendedPrivateKey sk,
    const bool legacy,
    bool* didErr)
{
    bls::ExtendedPrivateKey* skPtr = (bls::ExtendedPrivateKey*)sk;
    bls::ExtendedPublicKey* pk = nullptr;
    try {
        pk = new bls::ExtendedPublicKey(skPtr->GetExtendedPublicKey(legacy));
        *didErr = false;
    } catch (const std::exception& ex) {
        // set err
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    return pk;
}

void BIP32ExtendedPrivateKeyFree(const BIP32ExtendedPrivateKey sk)
{
    const bls::ExtendedPrivateKey* skPtr = (bls::ExtendedPrivateKey*)sk;
    delete skPtr;
}
