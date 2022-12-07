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

#include <vector>
#include "dashbls/bls.hpp"
#include "privatekey.h"
#include "blschia.h"
#include "error.h"
#include "utils.hpp"

// private key bindings implementation
CPrivateKey CPrivateKeyFromBytes(const void* data, const bool modOrder, bool* didErr) {
    bls::PrivateKey* skPtr = nullptr;
    try {
        skPtr = new bls::PrivateKey(
            bls::PrivateKey::FromBytes(
                bls::Bytes((uint8_t*)data, bls::PrivateKey::PRIVATE_KEY_SIZE),
                modOrder
            )
        );
    } catch (const std::exception& ex) {
        // set err
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;
    return skPtr;
}

CPrivateKey CPrivateKeyAggregate(void** sks, const size_t len) {
    return new bls::PrivateKey(
        bls::PrivateKey::Aggregate(toBLSVector<bls::PrivateKey>(sks, len))
    );
}

void* CPrivateKeySerialize(const CPrivateKey sk) {
    const bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    uint8_t* buffer = bls::Util::SecAlloc<uint8_t>(bls::PrivateKey::PRIVATE_KEY_SIZE);
    skPtr->Serialize(buffer);
    return (void*)buffer;
}

size_t CPrivateKeySizeBytes() {
    return bls::PrivateKey::PRIVATE_KEY_SIZE;
}

bool CPrivateKeyIsEqual(const CPrivateKey sk1, const CPrivateKey sk2) {
    const bls::PrivateKey* sk1Ptr = (bls::PrivateKey*)sk1;
    const bls::PrivateKey* sk2Ptr = (bls::PrivateKey*)sk2;
    return *sk1Ptr == *sk2Ptr;
}

CG1Element CPrivateKeyGetG1Element(const CPrivateKey sk, bool* didErr) {
    bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    bls::G1Element* el = nullptr;
    try {
        el = new bls::G1Element(skPtr->GetG1Element());
        *didErr = false;
    } catch (const std::exception& ex) {
        // set err
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    return el;
}

CG2Element CPrivateKeyGetG2Element(const CPrivateKey sk, bool* didErr) {
    bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    bls::G2Element* el = nullptr;
    try {
        el = new bls::G2Element(skPtr->GetG2Element());
        *didErr = false;
    } catch (const std::exception& ex) {
        // set err
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    return el;
}

CG2Element CPrivateKeyGetG2Power(const CPrivateKey sk, const CG2Element el) {
    const bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    const bls::G2Element* elPtr = (bls::G2Element*)el;
    return new bls::G2Element(skPtr->GetG2Power(*elPtr));
}

CG2Element CPrivateKeySignG2(const CPrivateKey sk,
                             uint8_t* msg,
                             const size_t len,
                             const uint8_t* dst,
                             const size_t dstLen) {
    const bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    return new bls::G2Element(skPtr->SignG2(msg, len, dst, dstLen));
}

void CPrivateKeyFree(CPrivateKey sk) {
    bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    delete skPtr;
}
