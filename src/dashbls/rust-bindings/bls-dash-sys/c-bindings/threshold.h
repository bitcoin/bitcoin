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

#ifndef THRESHOLD_H_
#define THRESHOLD_H_
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include "privatekey.h"
#include "elements.h"
#ifdef __cplusplus
extern "C" {
#endif

const int HashSize = 32;

PrivateKey ThresholdPrivateKeyShare(void** sks, const size_t sksLen, const void* hash, bool* didErr);
PrivateKey ThresholdPrivateKeyRecover(void** sks,
                                        const size_t sksLen,
                                        void** hashes,
                                        const size_t hashesLen,
                                        bool* didErr);

G1Element ThresholdPublicKeyShare(void** pks, const size_t pksLen, const void* hash, bool* didErr);
G1Element ThresholdPublicKeyRecover(void** pks,
                                      const size_t pksLen,
                                      void** hashes,
                                      const size_t hashesLen,
                                      bool* didErr);

G2Element ThresholdSignatureShare(void** sigs, const size_t sigsLen, const void* hash, bool* didErr);
G2Element ThresholdSignatureRecover(void** sigs,
                                      const size_t sigsLen,
                                      void** hashes,
                                      const size_t hashesLen,
                                      bool* didErr);

G2Element ThresholdSign(const PrivateKey sk, const void* hash);
bool ThresholdVerify(const G1Element pk, const void* hash, const G2Element sig);

#ifdef __cplusplus
}
#endif
#endif  // THRESHOLD_H_
