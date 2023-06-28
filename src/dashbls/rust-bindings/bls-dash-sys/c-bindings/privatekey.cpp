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
#include "bls.hpp"
#include "privatekey.h"
#include "blschia.h"
#include "error.h"
#include "utils.hpp"

// private key bindings implementation
PrivateKey PrivateKeyFromBytes(const void* data, const bool modOrder, bool* didErr) {
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

PrivateKey PrivateKeyFromSeedBIP32(const void* data, const size_t len) {
    return new bls::PrivateKey(
        bls::PrivateKey::FromSeedBIP32(bls::Bytes((uint8_t*)data, len))
    );
}

PrivateKey PrivateKeyAggregate(void** sks, const size_t len) {
    return new bls::PrivateKey(
        bls::PrivateKey::Aggregate(toBLSVector<bls::PrivateKey>(sks, len))
    );
}

void* PrivateKeySerialize(const PrivateKey sk) {
    const bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    uint8_t* buffer = bls::Util::SecAlloc<uint8_t>(bls::PrivateKey::PRIVATE_KEY_SIZE);
    skPtr->Serialize(buffer);

    return (void*)buffer;
}

size_t PrivateKeySizeBytes() {
    return bls::PrivateKey::PRIVATE_KEY_SIZE;
}

bool PrivateKeyIsEqual(const PrivateKey sk1, const PrivateKey sk2) {
    const bls::PrivateKey* sk1Ptr = (bls::PrivateKey*)sk1;
    const bls::PrivateKey* sk2Ptr = (bls::PrivateKey*)sk2;
    return *sk1Ptr == *sk2Ptr;
}

G1Element PrivateKeyGetG1Element(const PrivateKey sk, bool* didErr) {
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

G2Element PrivateKeyGetG2Element(const PrivateKey sk, bool* didErr) {
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

G2Element PrivateKeyGetG2Power(const PrivateKey sk, const G2Element el) {
    const bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    const bls::G2Element* elPtr = (bls::G2Element*)el;
    return new bls::G2Element(skPtr->GetG2Power(*elPtr));
}

G2Element PrivateKeySignG2(const PrivateKey sk,
                             uint8_t* msg,
                             const size_t len,
                             const uint8_t* dst,
                             const size_t dstLen) {
    const bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    return new bls::G2Element(skPtr->SignG2(msg, len, dst, dstLen));
}

void PrivateKeyFree(PrivateKey sk) {
    bls::PrivateKey* skPtr = (bls::PrivateKey*)sk;
    delete skPtr;
}
