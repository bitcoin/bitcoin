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

#include "chiabls/bls.hpp"
#include "extendedprivatekey.h"
#include "privatekey.h"
#include "publickey.h"
#include "chaincode.h"
#include "error.h"

CExtendedPrivateKey CExtendedPrivateKeyFromSeed(void *seed, size_t len, bool *didErr) {
    bls::ExtendedPrivateKey* key;
    try {
        key = new bls::ExtendedPrivateKey(bls::ExtendedPrivateKey::FromSeed(static_cast<uint8_t*>(seed), len));
    } catch (const std::exception& ex) {
        // set err
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;
    return key;
}

CExtendedPrivateKey CExtendedPrivateKeyFromBytes(void *p, bool *didErr) {
    bls::ExtendedPrivateKey* key;
    try {
        key = new bls::ExtendedPrivateKey(bls::ExtendedPrivateKey::FromBytes(static_cast<uint8_t*>(p)));
    } catch (const std::exception& ex) {
        // set err
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;
    return key;
}

void CExtendedPrivateKeyFree(CExtendedPrivateKey inPtr) {
    bls::ExtendedPrivateKey* key = (bls::ExtendedPrivateKey*)inPtr;
    delete key;
}

void* CExtendedPrivateKeySerialize(CExtendedPrivateKey inPtr) {
    bls::ExtendedPrivateKey* key = (bls::ExtendedPrivateKey*)inPtr;

    uint8_t* buffer = bls::Util::SecAlloc<uint8_t>(
        bls::ExtendedPrivateKey::EXTENDED_PRIVATE_KEY_SIZE);

    key->Serialize(buffer);
    return static_cast<void*>(buffer);
}

int CExtendedPrivateKeySizeBytes() {
    return bls::ExtendedPrivateKey::EXTENDED_PRIVATE_KEY_SIZE;
}

CPublicKey CExtendedPrivateKeyGetPublicKey(CExtendedPrivateKey inPtr) {
    bls::ExtendedPrivateKey* key = (bls::ExtendedPrivateKey*)inPtr;
    bls::PublicKey* pk = new bls::PublicKey(key->GetPublicKey());
    return pk;
}

CChainCode CExtendedPrivateKeyGetChainCode(CExtendedPrivateKey inPtr) {
    bls::ExtendedPrivateKey* key = (bls::ExtendedPrivateKey*)inPtr;
    bls::ChainCode* cc = new bls::ChainCode(key->GetChainCode());
    return cc;
}

CExtendedPrivateKey CExtendedPrivateKeyPrivateChild(CExtendedPrivateKey inPtr, uint32_t i, bool *didErr) {
    bls::ExtendedPrivateKey* key = (bls::ExtendedPrivateKey*)inPtr;
    bls::ExtendedPrivateKey* child;
    try {
        child = new bls::ExtendedPrivateKey(key->PrivateChild(i));
    } catch (const std::exception& ex) {
        // set err
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;
    return child;
}

CExtendedPublicKey CExtendedPrivateKeyPublicChild(CExtendedPrivateKey inPtr, uint32_t i, bool *didErr) {
    bls::ExtendedPrivateKey* key = (bls::ExtendedPrivateKey*)inPtr;
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

CExtendedPublicKey CExtendedPrivateKeyGetExtendedPublicKey(
    CExtendedPrivateKey inPtr) {
    bls::ExtendedPrivateKey* key = (bls::ExtendedPrivateKey*)inPtr;
    bls::ExtendedPublicKey* extpub = new bls::ExtendedPublicKey(
        key->GetExtendedPublicKey());
    return extpub;
}

uint32_t CExtendedPrivateKeyGetVersion(CExtendedPrivateKey inPtr) {
    bls::ExtendedPrivateKey* key = (bls::ExtendedPrivateKey*)inPtr;
    return key->GetVersion();
}

uint8_t CExtendedPrivateKeyGetDepth(CExtendedPrivateKey inPtr) {
    bls::ExtendedPrivateKey* key = (bls::ExtendedPrivateKey*)inPtr;
    return key->GetDepth();
}

uint32_t CExtendedPrivateKeyGetParentFingerprint(CExtendedPrivateKey inPtr) {
    bls::ExtendedPrivateKey* key = (bls::ExtendedPrivateKey*)inPtr;
    return key->GetParentFingerprint();
}

uint32_t CExtendedPrivateKeyGetChildNumber(CExtendedPrivateKey inPtr) {
    bls::ExtendedPrivateKey* key = (bls::ExtendedPrivateKey*)inPtr;
    return key->GetChildNumber();
}

CPrivateKey CExtendedPrivateKeyGetPrivateKey(CExtendedPrivateKey inPtr) {
    bls::PrivateKey* key = new bls::PrivateKey(
        ((bls::ExtendedPrivateKey*)inPtr)->GetPrivateKey()
    );
    return key;
}

bool CExtendedPrivateKeyIsEqual(CExtendedPrivateKey aPtr,
    CExtendedPrivateKey bPtr) {
    bls::ExtendedPrivateKey* a = (bls::ExtendedPrivateKey*)aPtr;
    bls::ExtendedPrivateKey* b = (bls::ExtendedPrivateKey*)bPtr;

    return *a == *b;
}
