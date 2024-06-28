// Copyright (c) 2021 The Dash Core developers

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "schemes.h"

#include <vector>

#include "bls.hpp"
#include "blschia.h"
#include "elements.h"
#include "error.h"
#include "privatekey.h"
#include "utils.hpp"

// Implementation of bindings for CoreMPL class

PrivateKey CoreMPLKeyGen(
    const CoreMPL scheme,
    const void* seed,
    const size_t seedLen,
    bool* didErr)
{
    bls::CoreMPL* schemePtr = (bls::CoreMPL*)scheme;
    bls::PrivateKey* sk = nullptr;
    try {
        sk = new bls::PrivateKey(
            schemePtr->KeyGen(bls::Bytes((uint8_t*)seed, seedLen)));
    } catch (const std::exception& ex) {
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;
    return sk;
}

G1Element CoreMPLSkToG1(const CoreMPL scheme, const PrivateKey sk)
{
    bls::CoreMPL* schemePtr = (bls::CoreMPL*)scheme;
    const bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    return new bls::G1Element(schemePtr->SkToG1(*skPtr));
}

G2Element CoreMPLSign(
    CoreMPL scheme,
    const PrivateKey sk,
    const void* msg,
    const size_t msgLen)
{
    bls::CoreMPL* schemePtr = (bls::CoreMPL*)scheme;
    const bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    return new bls::G2Element(
        schemePtr->Sign(*skPtr, bls::Bytes((uint8_t*)msg, msgLen)));
}

bool CoreMPLVerify(
    const CoreMPL scheme,
    const G1Element pk,
    const void* msg,
    const size_t msgLen,
    const G2Element sig)
{
    bls::CoreMPL* schemePtr = (bls::CoreMPL*)scheme;
    const bls::G1Element* pkPtr = (bls::G1Element*)pk;
    const bls::G2Element* sigPtr = (bls::G2Element*)sig;
    return schemePtr->Verify(
        *pkPtr, bls::Bytes((uint8_t*)msg, msgLen), *sigPtr);
}

bool CoreMPLVerifySecure(
    const CoreMPL scheme,
    void** pks,
    const size_t pksLen,
    const G2Element sig,
    const void* msg,
    const size_t msgLen)
{
    bls::CoreMPL* schemePtr = (bls::CoreMPL*)scheme;
    const std::vector<bls::G1Element> vecPubKeys =
        toBLSVector<bls::G1Element>(pks, pksLen);
    const uint8_t* msgPtr = (uint8_t*)msg;
    const bls::G2Element* sigPtr = (bls::G2Element*)sig;
    return schemePtr->VerifySecure(
        vecPubKeys, *sigPtr, bls::Bytes(msgPtr, msgLen));
}

G1Element CoreMPLAggregatePubKeys(
    const CoreMPL scheme,
    void** pks,
    const size_t pksLen)
{
    bls::CoreMPL* schemePtr = (bls::CoreMPL*)scheme;
    return new bls::G1Element(
        schemePtr->Aggregate(toBLSVector<bls::G1Element>(pks, pksLen)));
}

G2Element CoreMPLAggregateSigs(
    const CoreMPL scheme,
    void** sigs,
    const size_t sigsLen)
{
    bls::CoreMPL* schemePtr = (bls::CoreMPL*)scheme;
    return new bls::G2Element(
        schemePtr->Aggregate(toBLSVector<bls::G2Element>(sigs, sigsLen)));
}

PrivateKey CoreMPLDeriveChildSk(
    const CoreMPL scheme,
    const PrivateKey sk,
    const uint32_t index)
{
    bls::CoreMPL* schemePtr = (bls::CoreMPL*)scheme;
    const bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    return new bls::PrivateKey(schemePtr->DeriveChildSk(*skPtr, index));
}

PrivateKey CoreMPLDeriveChildSkUnhardened(
    CoreMPL scheme,
    PrivateKey sk,
    uint32_t index)
{
    bls::CoreMPL* schemePtr = (bls::CoreMPL*)scheme;
    bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    return new bls::PrivateKey(
        schemePtr->DeriveChildSkUnhardened(*skPtr, index));
}

G1Element CoreMPLDeriveChildPkUnhardened(
    CoreMPL scheme,
    G1Element el,
    uint32_t index)
{
    bls::CoreMPL* schemePtr = (bls::CoreMPL*)scheme;
    bls::G1Element* elPtr = (bls::G1Element*)el;
    return new bls::G1Element(
        schemePtr->DeriveChildPkUnhardened(*elPtr, index));
}

bool CoreMPLAggregateVerify(
    const CoreMPL scheme,
    void** pks,
    const size_t pksLen,
    void** msgs,
    const void* msgsLens,
    const size_t msgsLen,
    const G2Element sig)
{
    bls::CoreMPL* schemePtr = (bls::CoreMPL*)scheme;
    const size_t* msgLensPtr = (size_t*)msgsLens;
    const bls::G2Element* sigPtr = (bls::G2Element*)sig;
    const std::vector<bls::G1Element> vecPubKeys =
        toBLSVector<bls::G1Element>(pks, pksLen);
    const std::vector<size_t> vecMsgsLens =
        std::vector<size_t>(msgLensPtr, msgLensPtr + msgsLen);
    const std::vector<bls::Bytes> vecMsgs =
        toVectorBytes(msgs, msgsLen, vecMsgsLens);
    return schemePtr->AggregateVerify(vecPubKeys, vecMsgs, *sigPtr);
}

// BasicSchemeMPL
BasicSchemeMPL NewBasicSchemeMPL() { return new bls::BasicSchemeMPL(); }

bool BasicSchemeMPLAggregateVerify(
    BasicSchemeMPL scheme,
    void** pks,
    const size_t pksLen,
    void** msgs,
    const void* msgsLens,
    const size_t msgsLen,
    const G2Element sig)
{
    bls::BasicSchemeMPL* schemePtr = (bls::BasicSchemeMPL*)scheme;
    const size_t* msgLensPtr = (size_t*)msgsLens;
    const bls::G2Element* sigPtr = (bls::G2Element*)sig;
    const std::vector<bls::G1Element> vecPubKeys =
        toBLSVector<bls::G1Element>(pks, pksLen);
    const std::vector<size_t> vecMsgsLens =
        std::vector<size_t>(msgLensPtr, msgLensPtr + msgsLen);
    const std::vector<bls::Bytes> vecMsgs =
        toVectorBytes(msgs, msgsLen, vecMsgsLens);
    return schemePtr->AggregateVerify(vecPubKeys, vecMsgs, *sigPtr);
}

void BasicSchemeMPLFree(BasicSchemeMPL scheme)
{
    bls::BasicSchemeMPL* schemePtr = (bls::BasicSchemeMPL*)scheme;
    delete schemePtr;
}

// AugSchemeMPL
AugSchemeMPL NewAugSchemeMPL() { return new bls::AugSchemeMPL(); }

G2Element AugSchemeMPLSign(
    const AugSchemeMPL scheme,
    const PrivateKey sk,
    const void* msg,
    const size_t msgLen)
{
    bls::AugSchemeMPL* schemePtr = (bls::AugSchemeMPL*)scheme;
    const bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    return new bls::G2Element(
        schemePtr->Sign(*skPtr, bls::Bytes((uint8_t*)msg, msgLen)));
}

G2Element AugSchemeMPLSignPrepend(
    const AugSchemeMPL scheme,
    const PrivateKey sk,
    const void* msg,
    const size_t msgLen,
    const G1Element prepPk)
{
    bls::AugSchemeMPL* schemePtr = (bls::AugSchemeMPL*)scheme;
    const bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    const bls::G1Element* prepPkPtr = (bls::G1Element*)prepPk;
    return new bls::G2Element(
        schemePtr->Sign(*skPtr, bls::Bytes((uint8_t*)msg, msgLen), *prepPkPtr));
}

bool AugSchemeMPLVerify(
    const AugSchemeMPL scheme,
    const G1Element pk,
    const void* msg,
    const size_t msgLen,
    const G2Element sig)
{
    bls::AugSchemeMPL* schemePtr = (bls::AugSchemeMPL*)scheme;
    const bls::G1Element* pkPtr = (bls::G1Element*)pk;
    const uint8_t* msgPtr = (uint8_t*)msg;
    const bls::G2Element* sigPtr = (bls::G2Element*)sig;
    return schemePtr->Verify(*pkPtr, bls::Bytes(msgPtr, msgLen), *sigPtr);
}

bool AugSchemeMPLAggregateVerify(
    const AugSchemeMPL scheme,
    void** pks,
    const size_t pksLen,
    void** msgs,
    const void* msgsLens,
    const size_t msgsLen,
    const G2Element sig)
{
    bls::AugSchemeMPL* schemePtr = (bls::AugSchemeMPL*)scheme;
    const size_t* msgLensPtr = (size_t*)msgsLens;
    const bls::G2Element* sigPtr = (bls::G2Element*)sig;
    const std::vector<bls::G1Element> vecPubKeys =
        toBLSVector<bls::G1Element>(pks, pksLen);
    const std::vector<size_t> vecMsgsLens =
        std::vector<size_t>(msgLensPtr, msgLensPtr + msgsLen);
    const std::vector<bls::Bytes> vecMsgs =
        toVectorBytes(msgs, msgsLen, vecMsgsLens);
    return schemePtr->AggregateVerify(vecPubKeys, vecMsgs, *sigPtr);
}

void AugSchemeMPLFree(AugSchemeMPL scheme)
{
    bls::AugSchemeMPL* schemePtr = (bls::AugSchemeMPL*)scheme;
    delete schemePtr;
}

// PopSchemeMPL
PopSchemeMPL NewPopSchemeMPL() { return new bls::PopSchemeMPL(); }

G2Element PopSchemeMPLPopProve(
    const PopSchemeMPL scheme,
    const PrivateKey sk)
{
    bls::PopSchemeMPL* schemePtr = (bls::PopSchemeMPL*)scheme;
    const bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    return new bls::G2Element(schemePtr->PopProve(*skPtr));
}

bool PopSchemeMPLPopVerify(
    const PopSchemeMPL scheme,
    const G1Element pk,
    const G2Element sig)
{
    bls::PopSchemeMPL* schemePtr = (bls::PopSchemeMPL*)scheme;
    const bls::G1Element* pkPtr = (bls::G1Element*)pk;
    const bls::G2Element* sigPtr = (bls::G2Element*)sig;
    return schemePtr->PopVerify(*pkPtr, *sigPtr);
}

bool PopSchemeMPLFastAggregateVerify(
    const PopSchemeMPL scheme,
    void** pks,
    const size_t pksLen,
    const void* msg,
    const size_t msgLen,
    const G2Element sig)
{
    bls::PopSchemeMPL* schemePtr = (bls::PopSchemeMPL*)scheme;
    const bls::G2Element* sigPtr = (bls::G2Element*)sig;
    const std::vector<bls::G1Element> vecPubKeys =
        toBLSVector<bls::G1Element>(pks, pksLen);
    return schemePtr->FastAggregateVerify(
        vecPubKeys, bls::Bytes((uint8_t*)msg, msgLen), *sigPtr);
}

void PopSchemeMPLFree(PopSchemeMPL scheme)
{
    bls::PopSchemeMPL* schemePtr = (bls::PopSchemeMPL*)scheme;
    delete schemePtr;
}

// LegacySchemeMPL
LegacySchemeMPL NewLegacySchemeMPL() { return new bls::LegacySchemeMPL(); }

G2Element LegacySchemeMPLSign(
    const LegacySchemeMPL scheme,
    const PrivateKey sk,
    const void* msg,
    const size_t msgLen)
{
    bls::LegacySchemeMPL* schemePtr = (bls::LegacySchemeMPL*)scheme;
    const bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    return new bls::G2Element(
        schemePtr->Sign(*skPtr, bls::Bytes((uint8_t*)msg, msgLen)));
}

bool LegacySchemeMPLVerify(
    const LegacySchemeMPL scheme,
    const G1Element pk,
    const void* msg,
    const size_t msgLen,
    const G2Element sig)
{
    bls::LegacySchemeMPL* schemePtr = (bls::LegacySchemeMPL*)scheme;
    const bls::G1Element* pkPtr = (bls::G1Element*)pk;
    const uint8_t* msgPtr = (uint8_t*)msg;
    const bls::G2Element* sigPtr = (bls::G2Element*)sig;
    return schemePtr->Verify(*pkPtr, bls::Bytes(msgPtr, msgLen), *sigPtr);
}

bool LegacySchemeMPLVerifySecure(
    const LegacySchemeMPL scheme,
    void** pks,
    const size_t pksLen,
    const G2Element sig,
    const void* msg,
    const size_t msgLen)
{
    bls::LegacySchemeMPL* schemePtr = (bls::LegacySchemeMPL*)scheme;
    const std::vector<bls::G1Element> vecPubKeys =
        toBLSVector<bls::G1Element>(pks, pksLen);
    const uint8_t* msgPtr = (uint8_t*)msg;
    const bls::G2Element* sigPtr = (bls::G2Element*)sig;
    // Because of scheme pointer it will call CoreMPL::VerifySecure with 'legacy' flag variant
    return schemePtr->VerifySecure(
        vecPubKeys, *sigPtr, bls::Bytes(msgPtr, msgLen));
}

bool LegacySchemeMPLAggregateVerify(
    const LegacySchemeMPL scheme,
    void** pks,
    const size_t pksLen,
    void** msgs,
    const void* msgsLens,
    const size_t msgsLen,
    const G2Element sig)
{
    bls::LegacySchemeMPL* schemePtr = (bls::LegacySchemeMPL*)scheme;
    const size_t* msgLensPtr = (size_t*)msgsLens;
    const bls::G2Element* sigPtr = (bls::G2Element*)sig;
    const std::vector<bls::G1Element> vecPubKeys =
        toBLSVector<bls::G1Element>(pks, pksLen);
    const std::vector<size_t> vecMsgsLens =
        std::vector<size_t>(msgLensPtr, msgLensPtr + msgsLen);
    const std::vector<bls::Bytes> vecMsgs =
        toVectorBytes(msgs, msgsLen, vecMsgsLens);
    return schemePtr->AggregateVerify(vecPubKeys, vecMsgs, *sigPtr);
}

void LegacySchemeMPLFree(LegacySchemeMPL scheme)
{
    bls::LegacySchemeMPL* schemePtr = (bls::LegacySchemeMPL*)scheme;
    delete schemePtr;
}
