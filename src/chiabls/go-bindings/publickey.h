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

#ifndef GO_BINDINGS_PUBLICKEY_H_
#define GO_BINDINGS_PUBLICKEY_H_
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef void* CPublicKey;

CPublicKey CPublicKeyFromBytes(void *p, bool *didErr);

CPublicKey CPublicKeyAggregateInsecure(void **keys, size_t len, bool *didErr);

CPublicKey CPublicKeyAggregate(void **keys, size_t len, bool *didErr);

CPublicKey CPublicKeyShare(void **publicKeys, size_t numPublicKeys, void *id, bool *didErr);

CPublicKey CPublicKeyRecover(void** publicKeys, void** ids, size_t nSize, bool* fErrorOut);

bool CPublicKeyIsEqual(CPublicKey aPtr, CPublicKey bPtr);

void* CPublicKeySerialize(CPublicKey inPtr);
void CPublicKeyFree(CPublicKey inPtr);
int CPublicKeySizeBytes();

uint32_t CPublicKeyGetFingerprint(CPublicKey inPtr);

#ifdef __cplusplus
}
#endif
#endif  // GO_BINDINGS_PUBLICKEY_H_
