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

#include "chiabls/bls.hpp"
#include "extendedpublickey.h"
#include "publickey.h"
#include "chaincode.h"
#include "error.h"

CExtendedPublicKey CExtendedPublicKeyFromBytes(void *p, bool *didErr) {
    bls::ExtendedPublicKey* key;
    try {
        key = new bls::ExtendedPublicKey(bls::ExtendedPublicKey::FromBytes(static_cast<uint8_t*>(p)));
    } catch (const std::exception& ex) {
        // set err
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;
    return key;
}

void CExtendedPublicKeyFree(CExtendedPublicKey inPtr) {
    bls::ExtendedPublicKey* key = (bls::ExtendedPublicKey*)inPtr;
    delete key;
}

void* CExtendedPublicKeySerialize(CExtendedPublicKey inPtr) {
    bls::ExtendedPublicKey* key = (bls::ExtendedPublicKey*)inPtr;

    uint8_t* buffer = static_cast<uint8_t *>(malloc(
        bls::ExtendedPublicKey::EXTENDED_PUBLIC_KEY_SIZE));

    key->Serialize(buffer);
    return static_cast<void*>(buffer);
}

int CExtendedPublicKeySizeBytes() {
    return bls::ExtendedPublicKey::EXTENDED_PUBLIC_KEY_SIZE;
}

CExtendedPublicKey CExtendedPublicKeyPublicChild(CExtendedPublicKey inPtr, uint32_t i, bool *didErr) {
    bls::ExtendedPublicKey* key = (bls::ExtendedPublicKey*)inPtr;
    bls::ExtendedPublicKey* child;
    try {
        child = new bls::ExtendedPublicKey(key->PublicChild(i));
    } catch (const std::exception& ex) {
        // set err
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;
    return child;
}

CPublicKey CExtendedPublicKeyGetPublicKey(CExtendedPublicKey inPtr) {
    bls::ExtendedPublicKey* key = (bls::ExtendedPublicKey*)inPtr;
    bls::PublicKey* pk = new bls::PublicKey(key->GetPublicKey());
    return pk;
}

uint32_t CExtendedPublicKeyGetVersion(CExtendedPublicKey inPtr) {
    bls::ExtendedPublicKey* key = (bls::ExtendedPublicKey*)inPtr;
    return key->GetVersion();
}

uint8_t CExtendedPublicKeyGetDepth(CExtendedPublicKey inPtr) {
    bls::ExtendedPublicKey* key = (bls::ExtendedPublicKey*)inPtr;
    return key->GetDepth();
}

uint32_t CExtendedPublicKeyGetParentFingerprint(CExtendedPublicKey inPtr) {
    bls::ExtendedPublicKey* key = (bls::ExtendedPublicKey*)inPtr;
    return key->GetParentFingerprint();
}

uint32_t CExtendedPublicKeyGetChildNumber(CExtendedPublicKey inPtr) {
    bls::ExtendedPublicKey* key = (bls::ExtendedPublicKey*)inPtr;
    return key->GetChildNumber();
}

CChainCode CExtendedPublicKeyGetChainCode(CExtendedPublicKey inPtr) {
    bls::ExtendedPublicKey* key = (bls::ExtendedPublicKey*)inPtr;
    bls::ChainCode *cc = new bls::ChainCode(key->GetChainCode());
    return cc;
}

bool CExtendedPublicKeyIsEqual(CExtendedPublicKey aPtr,
    CExtendedPublicKey bPtr) {
    bls::ExtendedPublicKey *a = (bls::ExtendedPublicKey*)aPtr;
    bls::ExtendedPublicKey *b = (bls::ExtendedPublicKey*)bPtr;

    return *a == *b;
}
