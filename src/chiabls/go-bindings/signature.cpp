// Copyright (c) 2020 The Dash Core developers

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "signature.h"
#include <vector>
#include "chiabls/bls.hpp"
#include "error.h"

void* CInsecureSignatureSerialize(CInsecureSignature inPtr) {
    bls::InsecureSignature* sig = (bls::InsecureSignature*)inPtr;

    uint8_t* buffer = static_cast<uint8_t*>(malloc(
        bls::InsecureSignature::SIGNATURE_SIZE));

    sig->Serialize(buffer);
    return static_cast<void*>(buffer);
}

void CInsecureSignatureFree(CInsecureSignature inPtr) {
    bls::InsecureSignature* sig = (bls::InsecureSignature*)inPtr;
    delete sig;
}

int CInsecureSignatureSizeBytes() {
    return bls::InsecureSignature::SIGNATURE_SIZE;
}

CInsecureSignature CInsecureSignatureFromBytes(void *p, bool *didErr) {
    bls::InsecureSignature* sigPtr;
    try {
        sigPtr = new bls::InsecureSignature(
            bls::InsecureSignature::FromBytes(static_cast<uint8_t*>(p))
        );
    } catch (const std::exception& ex) {
        // set err
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;
    return sigPtr;
}

void* CSignatureSerialize(CSignature inPtr) {
    bls::Signature* sig = (bls::Signature*)inPtr;

    uint8_t* buffer = static_cast<uint8_t*>(
        malloc(bls::Signature::SIGNATURE_SIZE));

    sig->Serialize(buffer);
    return static_cast<void*>(buffer);
}

void CSignatureFree(CSignature inPtr) {
    bls::Signature* sig = (bls::Signature*)inPtr;
    delete sig;
}

int CSignatureSizeBytes() {
    return bls::Signature::SIGNATURE_SIZE;
}

CSignature CSignatureFromBytes(void *p, bool *didErr) {
    bls::Signature* sigPtr;
    try {
        sigPtr = new bls::Signature(
            bls::Signature::FromBytes(static_cast<uint8_t*>(p))
        );
    } catch (const std::exception& ex) {
        // set err
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;
    return sigPtr;
}

bool CSignatureVerify(CSignature inPtr) {
    bls::Signature* sig = (bls::Signature*)inPtr;

    bool didVerify;
    try {
        didVerify = sig->Verify();
    } catch (const std::exception& ex) {
        // set err
        gErrMsg = ex.what();
        didVerify = false;
    }
    return didVerify;
}

void CSignatureSetAggregationInfo(CSignature inPtr, CAggregationInfo aiPtr) {
    bls::Signature* sig = (bls::Signature*)inPtr;
    bls::AggregationInfo* ai = (bls::AggregationInfo*)aiPtr;

    const bls::AggregationInfo& aiRef = *ai;

    sig->SetAggregationInfo(aiRef);
    return;
}

CAggregationInfo CSignatureGetAggregationInfo(CSignature inPtr) {
    bls::Signature* sig = (bls::Signature*)inPtr;
    return new bls::AggregationInfo(*sig->GetAggregationInfo());
}

CSignature CSignatureAggregate(void **sigs, size_t len, bool *didErr) {
    std::vector<bls::Signature> vecSigs;
    for (int i = 0 ; i < len ; i++) {
        bls::Signature* sig = (bls::Signature*)sigs[i];
        vecSigs.push_back(*sig);
    }

    bls::Signature* sPtr;
    try {
        bls::Signature s = bls::Signature::AggregateSigs(vecSigs);
        sPtr = new bls::Signature(s);
    } catch (const std::exception& ex) {
        // set err
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;

    return sPtr;
}

CSignature CSignatureDivideBy(CSignature inPtr, void **sigs, size_t len,
    bool *didErr) {
    bls::Signature* sig = (bls::Signature*)inPtr;

    std::vector<bls::Signature> vecSigs;
    for (int i = 0 ; i < len ; i++) {
        bls::Signature* sig = (bls::Signature*)sigs[i];
        vecSigs.push_back(*sig);
    }

    bls::Signature* quotient;
    try {
        quotient = new bls::Signature(
            sig->DivideBy(vecSigs)
        );
    } catch (const std::exception& ex) {
        // set err
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;

    return quotient;
}

bool CInsecureSignatureVerify(CInsecureSignature inPtr, void **hashes,
    size_t numHashes, void **publicKeys, size_t numPublicKeys) {
    bls::InsecureSignature *sig = (bls::InsecureSignature*)inPtr;

    // build hashes vector
    std::vector<const uint8_t*> vecHashes;
    for (int i = 0; i < numHashes; i++) {
        const uint8_t* msg = (const uint8_t*)hashes[i];
        vecHashes.push_back(msg);
    }

    // build pubkeys vector
    std::vector<bls::PublicKey> vecPubKeys;
    for (int i = 0; i < numPublicKeys; i++) {
        bls::PublicKey* key = (bls::PublicKey*)publicKeys[i];
        vecPubKeys.push_back(*key);
    }

    bool didVerify;
    try {
        didVerify = sig->Verify(vecHashes, vecPubKeys);
    } catch (const std::exception& ex) {
        // set err
        gErrMsg = ex.what();
        didVerify = false;
    }
    return didVerify;
}

CInsecureSignature CInsecureSignatureAggregate(void **signatures,
    size_t numSignatures, bool *didErr) {
    // build the signatures vector
    std::vector<bls::InsecureSignature> vecSigs;
    for (int i = 0 ; i < numSignatures; i++) {
        bls::InsecureSignature* sig = (bls::InsecureSignature*)signatures[i];
        vecSigs.push_back(*sig);
    }

    bls::InsecureSignature *sig;
    try {
        sig = new bls::InsecureSignature(
            bls::InsecureSignature::Aggregate(vecSigs)
        );
    } catch (const std::exception& ex) {
        // set err
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;

    return sig;
}

CInsecureSignature CInsecureSignatureShare(void **signatures, size_t numSignatures, void *id, bool *didErr) {

    std::vector<bls::InsecureSignature> vecSignatures;
    for (int i = 0 ; i < numSignatures; ++i) {
        bls::InsecureSignature* sig = (bls::InsecureSignature*)signatures[i];
        vecSignatures.push_back(*sig);
    }

    bls::InsecureSignature* sig;
    try {
        sig = new bls::InsecureSignature(bls::BLS::SignatureShare(vecSignatures, (const uint8_t*)id));
    } catch (const std::exception& ex) {
        // set err
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;

    return sig;
}

CInsecureSignature CInsecureSignatureRecover(void** signatures, void** ids, size_t nSize, bool* fErrorOut)
{
    std::vector<bls::InsecureSignature> vecSigs;
    std::vector<const uint8_t*> vecIds;

    vecSigs.reserve(nSize);
    vecIds.reserve(nSize);

    for (int i = 0; i < nSize; ++i) {
        bls::InsecureSignature* sig = static_cast<bls::InsecureSignature*>(signatures[i]);
        vecSigs.push_back(*sig);
        vecIds.push_back(static_cast<const uint8_t*>(ids[i]));
    }

    bls::InsecureSignature* recoveredSig;
    try {
        recoveredSig = new bls::InsecureSignature(bls::BLS::RecoverSig(vecSigs, vecIds));
    } catch (const std::exception& ex) {
        gErrMsg = ex.what();
        *fErrorOut = true;
        return nullptr;
    }
    *fErrorOut = false;

    return recoveredSig;
}

CInsecureSignature CInsecureSignatureDivideBy(CInsecureSignature inPtr,
    void **signatures, size_t numSignatures, bool *didErr) {
    // build the signatures vector
    std::vector<bls::InsecureSignature> vecSigs;
    for (int i = 0 ; i < numSignatures; i++) {
        bls::InsecureSignature* sig = (bls::InsecureSignature*)signatures[i];
        vecSigs.push_back(*sig);
    }

    bls::InsecureSignature *sig = (bls::InsecureSignature*)inPtr;
    bls::InsecureSignature* quotient;

    try {
        quotient = new bls::InsecureSignature(
            sig->DivideBy(vecSigs)
        );
    } catch (const std::exception& ex) {
        // set err
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;

    return quotient;
}

bool CInsecureSignatureIsEqual(CInsecureSignature aPtr,
    CInsecureSignature bPtr) {
    bls::InsecureSignature *a = (bls::InsecureSignature*)aPtr;
    bls::InsecureSignature *b = (bls::InsecureSignature*)bPtr;

    return *a == *b;
}

CSignature CSignatureFromBytesWithAggregationInfo(void *p,
    CAggregationInfo aiPtr, bool *didErr) {
    bls::Signature* sigPtr;
    try {
        sigPtr = new bls::Signature(
            bls::Signature::FromBytes(static_cast<uint8_t*>(p),
                *((bls::AggregationInfo*)aiPtr))
        );
    } catch (const std::exception& ex) {
        // set err
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;
    return sigPtr;
}

CSignature CSignatureFromInsecureSig(CInsecureSignature inPtr) {
    bls::Signature *sig = new bls::Signature(
        bls::Signature::FromInsecureSig(*((bls::InsecureSignature *)inPtr))
    );

    return sig;
}

CSignature CSignatureFromInsecureSigWithAggregationInfo(
    CInsecureSignature inPtr, CAggregationInfo aiPtr) {
    bls::Signature *sig = new bls::Signature(
        bls::Signature::FromInsecureSig(
            *((bls::InsecureSignature*)inPtr),
            *((bls::AggregationInfo*)aiPtr)
        )
    );
    return sig;
}

CInsecureSignature CSignatureGetInsecureSig(CSignature inPtr) {
    bls::InsecureSignature *sig = new bls::InsecureSignature(
        ((bls::Signature *)inPtr)->GetInsecureSig()
    );
    return sig;
}

bool CSignatureIsEqual(CSignature aPtr, CSignature bPtr) {
    bls::Signature *a = (bls::Signature*)aPtr;
    bls::Signature *b = (bls::Signature*)bPtr;

    return *a == *b;
}
