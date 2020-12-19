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

#include "aggregationinfo.h"
#include <vector>
#include "chiabls/bls.hpp"
#include "publickey.h"
#include "error.h"

CAggregationInfo CAggregationInfoFromMsg(CPublicKey pkPtr, void* message,
    size_t len) {
    bls::PublicKey *key = (bls::PublicKey*)pkPtr;

    bls::AggregationInfo* ai = new bls::AggregationInfo(
        bls::AggregationInfo::FromMsg(*key, (const uint8_t*)message, len)
    );
    return ai;
}

CAggregationInfo CAggregationInfoFromMsgHash(CPublicKey pkPtr, void *hash) {
    bls::PublicKey *key = (bls::PublicKey*)pkPtr;

    bls::AggregationInfo* ai = new bls::AggregationInfo(
        bls::AggregationInfo::FromMsgHash(*key, (const uint8_t*)hash)
    );
    return ai;
}

CAggregationInfo CAggregationInfoFromVectors(void **publicKeys,
    size_t numPublicKeys, void **messageHashes, size_t numMessageHashes,
    void **exponents, size_t numExponents, size_t *sizesExponents,
    bool *didErr) {

    // build the pubkey vector
    std::vector<bls::PublicKey> vecPubKeys;
    for (int i = 0; i < numPublicKeys; i++) {
        bls::PublicKey* key = (bls::PublicKey*)publicKeys[i];
        vecPubKeys.push_back(*key);
    }

    // build the message hash vector
    std::vector<uint8_t*> vecHashes;
    for (int i = 0; i < numMessageHashes; i++) {
        uint8_t* msg = static_cast<uint8_t*>(messageHashes[i]);
        vecHashes.push_back(msg);
    }

    // build the exponents vector
    std::vector<bn_t*> vecExponents;
    for (int i = 0; i < numExponents; i++) {
        bn_t *exp;
        bn_new(*exp);
        bn_read_bin(*exp, static_cast<uint8_t*>(exponents[i]),
            sizesExponents[i]);
        vecExponents.push_back(exp);
    }

    bls::AggregationInfo* ai;
    try {
        ai = new bls::AggregationInfo(
            bls::AggregationInfo::FromVectors(vecPubKeys, vecHashes,
                vecExponents)
        );
    } catch (const std::exception& ex) {
        // set err
        gErrMsg = ex.what();
        *didErr = true;
        return nullptr;
    }
    *didErr = false;

    return ai;
}

void CAggregationInfoFree(CAggregationInfo inPtr) {
    bls::AggregationInfo* key = (bls::AggregationInfo*)inPtr;
    delete key;
}

CAggregationInfo MergeAggregationInfos(void **agginfos, size_t len) {
    std::vector<bls::AggregationInfo> vecAIs;

    for (int i = 0; i < len; i++) {
        bls::AggregationInfo* ai = (bls::AggregationInfo*)agginfos[i];
        vecAIs.push_back(*ai);
    }

    bls::AggregationInfo* ai = new bls::AggregationInfo(
        bls::AggregationInfo::MergeInfos(vecAIs)
    );

    return ai;
}

void CAggregationInfoRemoveEntries(CAggregationInfo inPtr, void **messages,
    size_t numMessages, void **publicKeys, size_t numPublicKeys, bool *didErr) {
    bls::AggregationInfo* ai = (bls::AggregationInfo*)inPtr;

    // build the message vector
    std::vector<uint8_t*> vecMessages;
    for (int i = 0; i < numMessages; i++) {
        uint8_t* msg = static_cast<uint8_t*>(messages[i]);
        vecMessages.push_back(msg);
    }

    // build the pubkey vector
    std::vector<bls::PublicKey> vecPubKeys;
    for (int i = 0; i < numPublicKeys; i++) {
        bls::PublicKey* key = (bls::PublicKey*)publicKeys[i];
        vecPubKeys.push_back(*key);
    }

    try {
        ai->RemoveEntries(vecMessages, vecPubKeys);
    } catch (const std::exception& ex) {
        // set err
        gErrMsg = ex.what();
        *didErr = true;
    }
    *didErr = false;

    return;
}

bool CAggregationInfoIsEqual(CAggregationInfo aPtr, CAggregationInfo bPtr) {
    bls::AggregationInfo* a = (bls::AggregationInfo*)aPtr;
    bls::AggregationInfo* b = (bls::AggregationInfo*)bPtr;

    return *a == *b;
}

bool CAggregationInfoIsLess(CAggregationInfo aPtr, CAggregationInfo bPtr) {
    bls::AggregationInfo* a = (bls::AggregationInfo*)aPtr;
    bls::AggregationInfo* b = (bls::AggregationInfo*)bPtr;

    return *a < *b;
}

bool CAggregationInfoEmpty(CAggregationInfo inPtr) {
    bls::AggregationInfo *ai = (bls::AggregationInfo*)inPtr;
    return ai->Empty();
}

uint8_t* CAggregationInfoGetPubKeys(CAggregationInfo inPtr,
    size_t *retNumKeys) {
    bls::AggregationInfo *ai = (bls::AggregationInfo*)inPtr;
    std::vector<bls::PublicKey> keys = ai->GetPubKeys();
    *retNumKeys = keys.size();

    uint8_t *buffer = static_cast<uint8_t*>(
        malloc(bls::PublicKey::PUBLIC_KEY_SIZE * (*retNumKeys)));
    for (int i = 0; i < *retNumKeys; ++i) {
        keys[i].Serialize(&buffer[i * bls::PublicKey::PUBLIC_KEY_SIZE]);
    }

    return buffer;
}

uint8_t* CAggregationInfoGetMessageHashes(CAggregationInfo inPtr,
    size_t *retNumHashes) {
    bls::AggregationInfo *ai = (bls::AggregationInfo*)inPtr;
    auto hashes = ai->GetMessageHashes();
    *retNumHashes = hashes.size();

    uint8_t *buffer = static_cast<uint8_t*>(
        malloc(bls::BLS::MESSAGE_HASH_LEN * (*retNumHashes)));
    for (int i = 0; i < *retNumHashes; ++i) {
        memcpy(&buffer[i * bls::BLS::MESSAGE_HASH_LEN], hashes[i],
            bls::BLS::MESSAGE_HASH_LEN);
    }

    return buffer;
}

int CBLSMessageHashLen() {
    return static_cast<int>(bls::BLS::MESSAGE_HASH_LEN);
}

size_t CAggregationInfoGetLength(CAggregationInfo inPtr) {
    bls::AggregationInfo *ai = (bls::AggregationInfo*)inPtr;
    std::vector<bls::PublicKey> pubKeys = ai->GetPubKeys();
    return pubKeys.size();
}
