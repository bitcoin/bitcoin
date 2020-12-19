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

#ifndef GO_BINDINGS_EXTENDEDPUBLICKEY_H_
#define GO_BINDINGS_EXTENDEDPUBLICKEY_H_
#include <stdbool.h>
#include "publickey.h"
#include "chaincode.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void* CExtendedPublicKey;

CExtendedPublicKey CExtendedPublicKeyFromBytes(void *p, bool *didErr);

CExtendedPublicKey CExtendedPublicKeyPublicChild(CExtendedPublicKey inPtr, uint32_t i, bool *didErr);

uint32_t CExtendedPublicKeyGetVersion(CExtendedPublicKey inPtr);
uint8_t CExtendedPublicKeyGetDepth(CExtendedPublicKey inPtr);
uint32_t CExtendedPublicKeyGetParentFingerprint(CExtendedPublicKey inPtr);
uint32_t CExtendedPublicKeyGetChildNumber(CExtendedPublicKey inPtr);

CChainCode CExtendedPublicKeyGetChainCode(CExtendedPublicKey inPtr);
CPublicKey CExtendedPublicKeyGetPublicKey(CExtendedPublicKey inPtr);

bool CExtendedPublicKeyIsEqual(CExtendedPublicKey aPtr,
    CExtendedPublicKey bPtr);

void* CExtendedPublicKeySerialize(CExtendedPublicKey inPtr);
void CExtendedPublicKeyFree(CExtendedPublicKey inPtr);
int CExtendedPublicKeySizeBytes();

#ifdef __cplusplus
}
#endif
#endif  // GO_BINDINGS_EXTENDEDPUBLICKEY_H_
