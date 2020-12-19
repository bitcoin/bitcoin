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

#ifndef GO_BINDINGS_PRIVATEKEY_H_
#define GO_BINDINGS_PRIVATEKEY_H_
#include <stdbool.h>
#include "publickey.h"
#include "signature.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void* CPrivateKey;

CPrivateKey CPrivateKeyFromSeed(void *p, int size, bool *didErr);

CPrivateKey CPrivateKeyFromBytes(void *p, bool modOrder, bool *didErr);

CPublicKey CPrivateKeyGetPublicKey(CPrivateKey inPtr);

CPrivateKey CPrivateKeyAggregateInsecure(void **privateKeys,
    size_t numPrivateKeys, bool *didErr);

CPrivateKey CPrivateKeyAggregate(void **privateKeys, size_t numPrivateKeys,
    void **publicKeys, size_t numPublicKeys, bool *didErr);

CPrivateKey CPrivateKeyShare(void **privateKeys, size_t numPrivateKeys, void *id, bool *didErr);

CPrivateKey CPrivateKeyRecover(void** privateKeys, void** ids, size_t nSize, bool* fErrorOut);

bool CPrivateKeyIsEqual(CPrivateKey aPtr, CPrivateKey bPtr);

CInsecureSignature CPrivateKeySignInsecure(CPrivateKey inPtr, void *msg,
    size_t len);
CInsecureSignature CPrivateKeySignInsecurePrehashed(CPrivateKey inPtr,
    void *hash);

CSignature CPrivateKeySign(CPrivateKey inPtr, void *msg, size_t len);
CSignature CPrivateKeySignPrehashed(CPrivateKey inPtr, void* hash);

void* CPrivateKeySerialize(CPrivateKey inPtr);
void CPrivateKeyFree(CPrivateKey inPtr);
int CPrivateKeySizeBytes();

#ifdef __cplusplus
}
#endif
#endif  // GO_BINDINGS_PRIVATEKEY_H_
