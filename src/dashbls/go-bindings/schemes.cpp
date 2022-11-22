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

#include <vector>
#include "dashbls/bls.hpp"
#include "privatekey.h"
#include "elements.h"
#include "schemes.h"
#include "blschia.h"
#include "error.h"
#include "utils.hpp"

// Implementation of bindings for CoreMPL class

CPrivateKey CCoreMPLKeyGen(const CCoreMPL scheme, const void* seed, const size_t seedLen, bool* didErr) {
    bls::CoreMPL* schemePtr = (bls::CoreMPL*)scheme;
    bls::PrivateKey* sk = nullptr;
    try {
        sk = new bls::PrivateKey(schemePtr->KeyGen(bls::Bytes((uint8_t*)seed, seedLen)));
    } catch (const std::exception& ex) {
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;
    return sk;
}

CG1Element CCoreMPSkToG1(const CCoreMPL scheme, const CPrivateKey sk) {
    bls::CoreMPL* schemePtr = (bls::CoreMPL*)scheme;
    const bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    return new bls::G1Element(
        schemePtr->SkToG1(*skPtr)
    );
}

CG2Element CCoreMPLSign(CCoreMPL scheme, const CPrivateKey sk, const void* msg, const size_t msgLen) {
    bls::CoreMPL* schemePtr = (bls::CoreMPL*)scheme;
    const bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    return new bls::G2Element(
        schemePtr->Sign(*skPtr, bls::Bytes((uint8_t*)msg, msgLen))
    );
}

bool CCoreMPLVerify(const CCoreMPL scheme,
                    const CG1Element pk,
                    const void* msg,
                    const size_t msgLen,
                    const CG2Element sig) {
    bls::CoreMPL* schemePtr = (bls::CoreMPL*)scheme;
    const bls::G1Element* pkPtr = (bls::G1Element*)pk;
    const bls::G2Element* sigPtr = (bls::G2Element*)sig;
    return schemePtr->Verify(*pkPtr, bls::Bytes((uint8_t*)msg, msgLen), *sigPtr);
}

CG1Element CCoreMPLAggregatePubKeys(const CCoreMPL scheme, void** pks, const size_t pksLen) {
    bls::CoreMPL* schemePtr = (bls::CoreMPL*)scheme;
    return new bls::G1Element(schemePtr->Aggregate(toBLSVector<bls::G1Element>(pks, pksLen)));
}

CG2Element CCoreMPLAggregateSigs(const CCoreMPL scheme, void** sigs, const size_t sigsLen) {
    bls::CoreMPL* schemePtr = (bls::CoreMPL*)scheme;
    return new bls::G2Element(
        schemePtr->Aggregate(toBLSVector<bls::G2Element>(sigs, sigsLen))
    );
}

CPrivateKey CCoreMPLDeriveChildSk(const CCoreMPL scheme, const CPrivateKey sk, const uint32_t index) {
    bls::CoreMPL* schemePtr = (bls::CoreMPL*)scheme;
    const bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    return new bls::PrivateKey(schemePtr->DeriveChildSk(*skPtr, index));
}

CPrivateKey CCoreMPLDeriveChildSkUnhardened(CCoreMPL scheme, CPrivateKey sk, uint32_t index) {
    bls::CoreMPL* schemePtr = (bls::CoreMPL*)scheme;
    bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    return new bls::PrivateKey(schemePtr->DeriveChildSkUnhardened(*skPtr, index));
}

CG1Element CCoreMPLDeriveChildPkUnhardened(CCoreMPL scheme, CG1Element el, uint32_t index) {
    bls::CoreMPL* schemePtr = (bls::CoreMPL*)scheme;
    bls::G1Element* elPtr = (bls::G1Element*)el;
    return new bls::G1Element(schemePtr->DeriveChildPkUnhardened(*elPtr, index));
}

bool CCoreMPLAggregateVerify(const CCoreMPL scheme,
                             void** pks,
                             const size_t pksLen,
                             void** msgs,
                             const void* msgsLens,
                             const size_t msgsLen,
                             const CG2Element sig) {
    bls::CoreMPL* schemePtr = (bls::CoreMPL*)scheme;
    const size_t* msgLensPtr = (size_t*)msgsLens;
    const bls::G2Element* sigPtr = (bls::G2Element*)sig;
    const std::vector<bls::G1Element> vecPubKeys = toBLSVector<bls::G1Element>(pks, pksLen);
    const std::vector<size_t> vecMsgsLens = std::vector<size_t>(msgLensPtr, msgLensPtr + msgsLen);
    const std::vector<bls::Bytes> vecMsgs = toVectorBytes(msgs, msgsLen, vecMsgsLens);
    return schemePtr->AggregateVerify(vecPubKeys, vecMsgs, *sigPtr);
}

// BasicSchemeMPL
CBasicSchemeMPL NewCBasicSchemeMPL() {
    return new bls::BasicSchemeMPL();
}

bool CBasicSchemeMPLAggregateVerify(CBasicSchemeMPL scheme,
                                    void** pks,
                                    const size_t pksLen,
                                    void** msgs,
                                    const void* msgsLens,
                                    const size_t msgsLen,
                                    const CG2Element sig) {
    bls::BasicSchemeMPL* schemePtr = (bls::BasicSchemeMPL*)scheme;
    const size_t* msgLensPtr = (size_t*)msgsLens;
    const bls::G2Element* sigPtr = (bls::G2Element*)sig;
    const std::vector<bls::G1Element> vecPubKeys = toBLSVector<bls::G1Element>(pks, pksLen);
    const std::vector<size_t> vecMsgsLens = std::vector<size_t>(msgLensPtr, msgLensPtr + msgsLen);
    const std::vector<bls::Bytes> vecMsgs = toVectorBytes(msgs, msgsLen, vecMsgsLens);
    return schemePtr->AggregateVerify(vecPubKeys, vecMsgs, *sigPtr);
}

void CBasicSchemeMPLFree(CBasicSchemeMPL scheme) {
    bls::BasicSchemeMPL* schemePtr = (bls::BasicSchemeMPL*)scheme;
    delete schemePtr;
}

// AugSchemeMPL
CAugSchemeMPL NewCAugSchemeMPL() {
    return new bls::AugSchemeMPL();
}

CG2Element CAugSchemeMPLSign(const CAugSchemeMPL scheme, const CPrivateKey sk, const void* msg, const size_t msgLen) {
    bls::AugSchemeMPL* schemePtr = (bls::AugSchemeMPL*)scheme;
    const bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    return new bls::G2Element(
        schemePtr->Sign(*skPtr, bls::Bytes((uint8_t*)msg, msgLen))
    );
}

CG2Element CAugSchemeMPLSignPrepend(const CAugSchemeMPL scheme,
                                    const CPrivateKey sk,
                                    const void* msg,
                                    const size_t msgLen,
                                    const CG1Element prepPk) {
    bls::AugSchemeMPL* schemePtr = (bls::AugSchemeMPL*)scheme;
    const bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    const bls::G1Element* prepPkPtr = (bls::G1Element*)prepPk;
    return new bls::G2Element(schemePtr->Sign(*skPtr, bls::Bytes((uint8_t*)msg, msgLen), *prepPkPtr));
}

bool CAugSchemeMPLVerify(const CAugSchemeMPL scheme,
                         const CG1Element pk,
                         const void* msg,
                         const size_t msgLen,
                         const CG2Element sig) {
    bls::AugSchemeMPL* schemePtr = (bls::AugSchemeMPL*)scheme;
    const bls::G1Element* pkPtr = (bls::G1Element*)pk;
    const uint8_t* msgPtr = (uint8_t*)msg;
    const bls::G2Element* sigPtr = (bls::G2Element*)sig;
    return schemePtr->Verify(*pkPtr, bls::Bytes(msgPtr, msgLen), *sigPtr);
}

bool CAugSchemeMPLAggregateVerify(const CAugSchemeMPL scheme,
                                  void** pks,
                                  const size_t pksLen,
                                  void** msgs,
                                  const void* msgsLens,
                                  const size_t msgsLen,
                                  const CG2Element sig) {
    bls::AugSchemeMPL* schemePtr = (bls::AugSchemeMPL*)scheme;
    const size_t* msgLensPtr = (size_t*)msgsLens;
    const bls::G2Element* sigPtr = (bls::G2Element*)sig;
    const std::vector<bls::G1Element> vecPubKeys = toBLSVector<bls::G1Element>(pks, pksLen);
    const std::vector<size_t> vecMsgsLens = std::vector<size_t>(msgLensPtr, msgLensPtr + msgsLen);
    const std::vector<bls::Bytes> vecMsgs = toVectorBytes(msgs, msgsLen, vecMsgsLens);
    return schemePtr->AggregateVerify(vecPubKeys, vecMsgs, *sigPtr);
}

void CAugSchemeMPLFree(CAugSchemeMPL scheme) {
    bls::AugSchemeMPL* schemePtr = (bls::AugSchemeMPL*)scheme;
    delete schemePtr;
}

// PopSchemeMPL
CPopSchemeMPL NewCPopSchemeMPL() {
    return new bls::PopSchemeMPL();
}

CG2Element CPopSchemeMPLPopProve(const CPopSchemeMPL scheme, const CPrivateKey sk) {
    bls::PopSchemeMPL* schemePtr = (bls::PopSchemeMPL*)scheme;
    const bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    return new bls::G2Element(schemePtr->PopProve(*skPtr));
}

bool CPopSchemeMPLPopVerify(const CPopSchemeMPL scheme, const CG1Element pk, const CG2Element sig) {
    bls::PopSchemeMPL* schemePtr = (bls::PopSchemeMPL*)scheme;
    const bls::G1Element* pkPtr = (bls::G1Element*)pk;
    const bls::G2Element* sigPtr = (bls::G2Element*)sig;
    return schemePtr->PopVerify(*pkPtr, *sigPtr);
}

bool CPopSchemeMPLFastAggregateVerify(const CPopSchemeMPL scheme,
                                      void** pks,
                                      const size_t pksLen,
                                      const void* msg,
                                      const size_t msgLen,
                                      const CG2Element sig) {
    bls::PopSchemeMPL* schemePtr = (bls::PopSchemeMPL*)scheme;
    const bls::G2Element* sigPtr = (bls::G2Element*)sig;
    const std::vector<bls::G1Element> vecPubKeys = toBLSVector<bls::G1Element>(pks, pksLen);
    return schemePtr->FastAggregateVerify(vecPubKeys, bls::Bytes((uint8_t*)msg, msgLen), *sigPtr);
}

void CPopSchemeMPLFree(CPopSchemeMPL scheme) {
    bls::PopSchemeMPL* schemePtr = (bls::PopSchemeMPL*)scheme;
    delete schemePtr;
}
