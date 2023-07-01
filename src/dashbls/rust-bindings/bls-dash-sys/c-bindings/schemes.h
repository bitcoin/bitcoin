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

#ifndef SCHEMES_H_
#define SCHEMES_H_
#include <stdbool.h>
#include <stdlib.h>

#include "elements.h"
#include "privatekey.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void* CoreMPL;
typedef CoreMPL BasicSchemeMPL;
typedef CoreMPL AugSchemeMPL;
typedef CoreMPL PopSchemeMPL;
typedef CoreMPL LegacySchemeMPL;

// CoreMPL
PrivateKey CoreMPLKeyGen(
    const CoreMPL scheme,
    const void* seed,
    const size_t seedLen,
    bool* didErr);
G1Element CoreMPLSkToG1(const CoreMPL scheme, const PrivateKey sk);
G2Element CoreMPLSign(
    const CoreMPL scheme,
    const PrivateKey sk,
    const void* msg,
    const size_t msgLen);
bool CoreMPLVerify(
    const BasicSchemeMPL scheme,
    const G1Element pk,
    const void* msg,
    const size_t msgLen,
    const G2Element sig);
bool CoreMPLVerifySecure(
    const CoreMPL scheme,
    void** pks,
    const size_t pksLen,
    const G2Element sig,
    const void* msg,
    const size_t msgLen);
G1Element CoreMPLAggregatePubKeys(
    const CoreMPL scheme,
    void** pubKeys,
    const size_t pkLen);
G2Element CoreMPLAggregateSigs(
    const CoreMPL scheme,
    void** sigs,
    const size_t sigLen);
PrivateKey CoreMPLDeriveChildSk(
    const CoreMPL scheme,
    const PrivateKey sk,
    const uint32_t index);
PrivateKey CoreMPLDeriveChildSkUnhardened(
    const CoreMPL scheme,
    const PrivateKey sk,
    const uint32_t index);
G1Element CoreMPLDeriveChildPkUnhardened(
    const CoreMPL scheme,
    const G1Element sk,
    const uint32_t index);
bool CoreMPLAggregateVerify(
    const CoreMPL scheme,
    void** pks,
    const size_t pkLen,
    void** msgs,
    const void* msgLens,
    const size_t msgLen,
    const G2Element sig);

// BasicSchemeMPL
BasicSchemeMPL NewBasicSchemeMPL();
bool BasicSchemeMPLAggregateVerify(
    BasicSchemeMPL scheme,
    void** pks,
    const size_t pksLen,
    void** msgs,
    const void* msgsLens,
    const size_t msgsLen,
    const G2Element sig);
void BasicSchemeMPLFree(BasicSchemeMPL scheme);

// AugSchemeMPL
AugSchemeMPL NewAugSchemeMPL();
G2Element AugSchemeMPLSign(
    const AugSchemeMPL scheme,
    const PrivateKey sk,
    const void* msg,
    const size_t msgLen);
G2Element AugSchemeMPLSignPrepend(
    const AugSchemeMPL scheme,
    const PrivateKey sk,
    const void* msg,
    const size_t msgLen,
    const G1Element prepPk);
bool AugSchemeMPLVerify(
    const AugSchemeMPL scheme,
    const G1Element pk,
    const void* msg,
    const size_t msgLen,
    const G2Element sig);
bool AugSchemeMPLAggregateVerify(
    const AugSchemeMPL scheme,
    void** pks,
    const size_t pksLen,
    void** msgs,
    const void* msgsLens,
    const size_t msgsLen,
    const G2Element sig);
void AugSchemeMPLFree(AugSchemeMPL scheme);

// PopSchemeMPL
PopSchemeMPL NewPopSchemeMPL();
G2Element PopSchemeMPLPopProve(
    const PopSchemeMPL scheme,
    const PrivateKey sk);
bool PopSchemeMPLPopVerify(
    const PopSchemeMPL scheme,
    const G1Element pk,
    const G2Element sig);
bool PopSchemeMPLFastAggregateVerify(
    const PopSchemeMPL scheme,
    void** pks,
    const size_t pksLen,
    const void* msgs,
    const size_t msgsLen,
    const G2Element sig);
void PopSchemeMPLFree(PopSchemeMPL scheme);

// LegacySchemeMPL
LegacySchemeMPL NewLegacySchemeMPL();
G2Element LegacySchemeMPLSign(
    const LegacySchemeMPL scheme,
    const PrivateKey sk,
    const void* msg,
    const size_t msgLen);
G2Element LegacySchemeMPLSignPrepend(
    const LegacySchemeMPL scheme,
    const PrivateKey sk,
    const void* msg,
    const size_t msgLen,
    const G1Element prepPk);
bool LegacySchemeMPLVerify(
    const LegacySchemeMPL scheme,
    const G1Element pk,
    const void* msg,
    const size_t msgLen,
    const G2Element sig);
bool LegacySchemeMPLVerifySecure(
    const LegacySchemeMPL scheme,
    void** pks,
    const size_t pksLen,
    const G2Element sig,
    const void* msg,
    const size_t msgLen);
bool LegacySchemeMPLAggregateVerify(
    const LegacySchemeMPL scheme,
    void** pks,
    const size_t pksLen,
    void** msgs,
    const void* msgsLens,
    const size_t msgsLen,
    const G2Element sig);
void LegacySchemeMPLFree(LegacySchemeMPL scheme);

#ifdef __cplusplus
}
#endif
#endif  // SCHEMES_H_
