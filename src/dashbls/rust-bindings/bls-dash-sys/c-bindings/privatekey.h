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

#ifndef PRIVATEKEY_H_
#define PRIVATEKEY_H_
#include <stdbool.h>
#include <stdlib.h>
#include "elements.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void* PrivateKey;

PrivateKey PrivateKeyFromBytes(const void* data, const bool modOrder, bool* didErr);
PrivateKey PrivateKeyFromSeedBIP32(const void* data, const size_t len);
PrivateKey PrivateKeyAggregate(void** sks, const size_t len);
G1Element PrivateKeyGetG1Element(const PrivateKey sk, bool* didErr);
G2Element PrivateKeyGetG2Element(const PrivateKey sk, bool* didErr);
G2Element PrivateKeyGetG2Power(const PrivateKey sk, const G2Element el);
bool PrivateKeyIsEqual(const PrivateKey sk1, const PrivateKey sk2);
void* PrivateKeySerialize(const PrivateKey sk);
void PrivateKeyFree(const PrivateKey sk);
size_t PrivateKeySizeBytes();

#ifdef __cplusplus
}
#endif
#endif  // PRIVATEKEY_H_
