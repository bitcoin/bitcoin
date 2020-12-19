// Copyright 2019 Chia Network Inc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef GO_BINDINGS_EXTENDEDPRIVATEKEY_H_
#define GO_BINDINGS_EXTENDEDPRIVATEKEY_H_
#include <stdbool.h>
#include "chaincode.h"
#include "extendedpublickey.h"
#include "privatekey.h"
#include "publickey.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void* CExtendedPrivateKey;

CExtendedPrivateKey CExtendedPrivateKeyFromSeed(void *seed, size_t len, bool *didErr);

CExtendedPrivateKey CExtendedPrivateKeyFromBytes(void *p, bool *didErr);

CExtendedPrivateKey CExtendedPrivateKeyPrivateChild(CExtendedPrivateKey inPtr, uint32_t i, bool *didErr);

CExtendedPublicKey CExtendedPrivateKeyPublicChild(CExtendedPrivateKey inPtr, uint32_t i, bool *didErr);

uint32_t CExtendedPrivateKeyGetVersion(CExtendedPrivateKey inPtr);
uint8_t CExtendedPrivateKeyGetDepth(CExtendedPrivateKey inPtr);
uint32_t CExtendedPrivateKeyGetParentFingerprint(CExtendedPrivateKey inPtr);
uint32_t CExtendedPrivateKeyGetChildNumber(CExtendedPrivateKey inPtr);

CChainCode CExtendedPrivateKeyGetChainCode(CExtendedPrivateKey inPtr);
CPrivateKey CExtendedPrivateKeyGetPrivateKey(CExtendedPrivateKey inPtr);

CPublicKey CExtendedPrivateKeyGetPublicKey(CExtendedPrivateKey inPtr);
CExtendedPublicKey CExtendedPrivateKeyGetExtendedPublicKey(
    CExtendedPrivateKey inPtr);

bool CExtendedPrivateKeyIsEqual(CExtendedPrivateKey aPtr,
    CExtendedPrivateKey bPtr);

void* CExtendedPrivateKeySerialize(CExtendedPrivateKey inPtr);
void CExtendedPrivateKeyFree(CExtendedPrivateKey inPtr);
int CExtendedPrivateKeySizeBytes();

#ifdef __cplusplus
}
#endif
#endif  // GO_BINDINGS_EXTENDEDPRIVATEKEY_H_
