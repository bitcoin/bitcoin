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

#ifndef GO_BINDINGS_SCHEMES_H_
#define GO_BINDINGS_SCHEMES_H_
#include <stdbool.h>
#include <stdlib.h>
#include "privatekey.h"
#include "elements.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void* CCoreMPL;
typedef CCoreMPL CBasicSchemeMPL;
typedef CCoreMPL CAugSchemeMPL;
typedef CCoreMPL CPopSchemeMPL;

// CoreMPL
CPrivateKey CCoreMPLKeyGen(const CCoreMPL scheme, const void* seed, const size_t seedLen, bool* didErr);
CG1Element CCoreMPSkToG1(const CCoreMPL scheme, const CPrivateKey sk);
CG2Element CCoreMPLSign(const CCoreMPL scheme, const CPrivateKey sk, const void* msg, const size_t msgLen);
bool CCoreMPLVerify(const CBasicSchemeMPL scheme,
                    const CG1Element pk,
                    const void* msg,
                    const size_t msgLen,
                    const CG2Element sig);
CG1Element CCoreMPLAggregatePubKeys(const CCoreMPL scheme, void** pubKeys, const size_t pkLen);
CG2Element CCoreMPLAggregateSigs(const CCoreMPL scheme, void** sigs, const size_t sigLen);
CPrivateKey CCoreMPLDeriveChildSk(const CCoreMPL scheme, const CPrivateKey sk, const uint32_t index);
CPrivateKey CCoreMPLDeriveChildSkUnhardened(const CCoreMPL scheme, const CPrivateKey sk, const uint32_t index);
CG1Element CCoreMPLDeriveChildPkUnhardened(const CCoreMPL scheme, const CG1Element sk, const uint32_t index);
bool CCoreMPLAggregateVerify(const CCoreMPL scheme,
                             void** pks,
                             const size_t pkLen,
                             void** msgs,
                             const void* msgLens,
                             const size_t msgLen,
                             const CG2Element sig);

// BasicSchemeMPL
CBasicSchemeMPL NewCBasicSchemeMPL();
bool CBasicSchemeMPLAggregateVerify(CBasicSchemeMPL scheme,
                                    void** pks,
                                    const size_t pksLen,
                                    void** msgs,
                                    const void* msgsLens,
                                    const size_t msgsLen,
                                    const CG2Element sig);
void CBasicSchemeMPLFree(CBasicSchemeMPL scheme);

// AugSchemeMPL
CAugSchemeMPL NewCAugSchemeMPL();
CG2Element CAugSchemeMPLSign(const CAugSchemeMPL scheme, const CPrivateKey sk, const void* msg, const size_t msgLen);
CG2Element CAugSchemeMPLSignPrepend(const CAugSchemeMPL scheme,
                                    const CPrivateKey sk,
                                    const void* msg,
                                    const size_t msgLen,
                                    const CG1Element prepPk);
bool CAugSchemeMPLVerify(const CAugSchemeMPL scheme,
                         const CG1Element pk,
                         const void* msg,
                         const size_t msgLen,
                         const CG2Element sig);
bool CAugSchemeMPLAggregateVerify(const CAugSchemeMPL scheme,
                                  void** pks,
                                  const size_t pksLen,
                                  void** msgs,
                                  const void* msgsLens,
                                  const size_t msgsLen,
                                  const CG2Element sig);
void CAugSchemeMPLFree(CAugSchemeMPL scheme);

// PopSchemeMPL
CPopSchemeMPL NewCPopSchemeMPL();
CG2Element CPopSchemeMPLPopProve(const CPopSchemeMPL scheme, const CPrivateKey sk);
bool CPopSchemeMPLPopVerify(const CPopSchemeMPL scheme, const CG1Element pk, const CG2Element sig);
bool CPopSchemeMPLFastAggregateVerify(const CPopSchemeMPL scheme,
                                      void** pks,
                                      const size_t pksLen,
                                      const void* msgs,
                                      const size_t msgsLen,
                                      const CG2Element sig);
void CPopSchemeMPLFree(CPopSchemeMPL scheme);

#ifdef __cplusplus
}
#endif
#endif  // GO_BINDINGS_SCHEMES_H_
