#include <vector>
#include "bls.hpp"
#include "chaincode.h"

void* BIP32ChainCodeSerialize(const BIP32ChainCode cc)
{
    const bls::ChainCode* ccPtr = (bls::ChainCode*)cc;
    const std::vector<uint8_t> serialized = ccPtr->Serialize();
    uint8_t* buffer = (uint8_t*)malloc(bls::ChainCode::SIZE);
    memcpy(buffer, serialized.data(), bls::ChainCode::SIZE);
    return (void*)buffer;
}

bool BIP32ChainCodeIsEqual(const BIP32ChainCode cc1, const BIP32ChainCode cc2)
{
    const bls::ChainCode* cc1Ptr = (bls::ChainCode*)cc1;
    const bls::ChainCode* cc2Ptr = (bls::ChainCode*)cc2;
    return *cc1Ptr == *cc2Ptr;
}

void BIP32ChainCodeFree(const BIP32ChainCode cc)
{
    const bls::ChainCode* ccPtr = (bls::ChainCode*)cc;
    delete ccPtr;
}
