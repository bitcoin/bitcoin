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

#ifndef GO_BINDINGS_SIGNATURE_H_
#define GO_BINDINGS_SIGNATURE_H_
#include <stdbool.h>
#include "aggregationinfo.h"
#include "publickey.h"
#ifdef __cplusplus
extern "C" {
#endif

// -- InsecureSignature

typedef void* CInsecureSignature;

CInsecureSignature CInsecureSignatureFromBytes(void *p, bool *didErr);

void* CInsecureSignatureSerialize(CInsecureSignature inPtr);
void CInsecureSignatureFree(CInsecureSignature inPtr);
int CInsecureSignatureSizeBytes();

bool CInsecureSignatureVerify(CInsecureSignature inPtr, void **hashes,
    size_t numHashes, void **publicKeys, size_t numPublicKeys);

CInsecureSignature CInsecureSignatureAggregate(void **signatures,
    size_t numSignatures, bool *didErr);

CInsecureSignature CInsecureSignatureShare(void **signatures, size_t numSignatures, void *id, bool *didErr);

CInsecureSignature CInsecureSignatureRecover(void** signatures, void** ids, size_t nSize, bool* fErrorOut);

CInsecureSignature CInsecureSignatureDivideBy(CInsecureSignature inPtr,
    void **signatures, size_t numSignatures, bool *didErr);

bool CInsecureSignatureIsEqual(CInsecureSignature aPtr,
    CInsecureSignature bPtr);

// -- Signature

typedef void* CSignature;

CSignature CSignatureFromBytes(void *p, bool *didErr);

CSignature CSignatureFromBytesWithAggregationInfo(void *p,
    CAggregationInfo aiPtr, bool *didErr);
CSignature CSignatureFromInsecureSig(CInsecureSignature inPtr);

CSignature CSignatureFromInsecureSigWithAggregationInfo(
    CInsecureSignature inPtr, CAggregationInfo aiPtr);
CInsecureSignature CSignatureGetInsecureSig(CSignature inPtr);

void* CSignatureSerialize(CSignature inPtr);
void CSignatureFree(CSignature inPtr);
int CSignatureSizeBytes();

bool CSignatureVerify(CSignature inPtr);

void CSignatureSetAggregationInfo(CSignature inPtr, CAggregationInfo aiPtr);

CAggregationInfo CSignatureGetAggregationInfo(CSignature inPtr);

CSignature CSignatureAggregate(void **sigs, size_t len, bool *didErr);

CSignature CSignatureDivideBy(CSignature inPtr, void **sigs, size_t len,
    bool *didErr);

bool CSignatureIsEqual(CSignature aPtr, CSignature bPtr);

#ifdef __cplusplus
}
#endif
#endif  // GO_BINDINGS_SIGNATURE_H_
