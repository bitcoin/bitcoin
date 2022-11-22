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

#ifndef GO_BINDINGS_PRIVATEKEY_H_
#define GO_BINDINGS_PRIVATEKEY_H_
#include <stdbool.h>
#include <stdlib.h>
#include "elements.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void* CPrivateKey;

CPrivateKey CPrivateKeyFromBytes(const void* data, const bool modOrder, bool* didErr);
CPrivateKey CPrivateKeyAggregate(void** sks, const size_t len);
CG1Element CPrivateKeyGetG1Element(const CPrivateKey sk, bool* didErr);
CG2Element CPrivateKeyGetG2Element(const CPrivateKey sk, bool* didErr);
CG2Element CPrivateKeyGetG2Power(const CPrivateKey sk, const CG2Element el);
bool CPrivateKeyIsEqual(const CPrivateKey sk1, const CPrivateKey sk2);
void* CPrivateKeySerialize(const CPrivateKey sk);
void CPrivateKeyFree(const CPrivateKey sk);
size_t CPrivateKeySizeBytes();

#ifdef __cplusplus
}
#endif
#endif  // GO_BINDINGS_PRIVATEKEY_H_
