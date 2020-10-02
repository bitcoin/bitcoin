// Copyright 2018 Chia Network Inc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string>
#include <cstring>
#include <set>
#include <algorithm>
#include <utility>
#include "aggregationinfo.hpp"
#include "bls.hpp"
namespace bls {

// Creates a new object, copying the messageHash and pk
AggregationInfo AggregationInfo::FromMsgHash(const PublicKey &pk,
                                             const uint8_t *messageHash) {
    uint8_t* mapKey = new uint8_t[BLS::MESSAGE_HASH_LEN +
                                  PublicKey::PUBLIC_KEY_SIZE];

    std::memcpy(mapKey, messageHash, BLS::MESSAGE_HASH_LEN);
    pk.Serialize(mapKey + BLS::MESSAGE_HASH_LEN);
    AggregationInfo::AggregationTree tree;
    bn_t *one = new bn_t[1];
    bn_new(*one);
    bn_zero(*one);
    bn_set_dig(*one, 1);
    tree.insert(std::make_pair(mapKey, one));

    std::vector<uint8_t*> hashes = {mapKey};
    std::vector<PublicKey> pks = {pk};

    return AggregationInfo(tree, hashes, pks);
}

AggregationInfo AggregationInfo::FromMsg(const PublicKey &pk,
                                         const uint8_t *message,
                                         size_t len) {
    uint8_t hash[BLS::MESSAGE_HASH_LEN];
    Util::Hash256(hash, message, len);
    return FromMsgHash(pk, hash);
}

AggregationInfo AggregationInfo::FromVectors(
        std::vector<PublicKey> const &pubKeys,
        std::vector<uint8_t*> const &messageHashes,
        std::vector<bn_t*> const &exponents) {
    if (pubKeys.size() != messageHashes.size() || messageHashes.size() !=
            exponents.size()) {
         throw std::string(("Invalid input, all std::vectors must have\
                             the same length"));
    }
    AggregationInfo::AggregationTree tree;
    for (size_t i = 0; i < pubKeys.size(); i++) {
        uint8_t * mapKey = new uint8_t[BLS::MESSAGE_HASH_LEN +
                                       PublicKey::PUBLIC_KEY_SIZE];
        std::memcpy(mapKey, messageHashes[i], BLS::MESSAGE_HASH_LEN);
        pubKeys[i].Serialize(mapKey + BLS::MESSAGE_HASH_LEN);
        bn_t *mapValue = new bn_t[1];
        bn_new(*mapValue)
        bn_copy(*mapValue, *exponents[i]);
        tree.insert(std::make_pair(mapKey, mapValue));
    }
    std::vector<PublicKey> sortedPubKeys;
    std::vector<uint8_t*> sortedMessageHashes;
    SortIntoVectors(sortedMessageHashes, sortedPubKeys, tree);
    return AggregationInfo(tree, sortedMessageHashes, sortedPubKeys);
}

// Merges multiple AggregationInfo objects together.
AggregationInfo AggregationInfo::MergeInfos(std::vector<AggregationInfo>
                                            const &infos) {
    // Find messages that are in multiple infos
    std::set<const uint8_t*, Util::BytesCompare32> messages;
    std::set<const uint8_t*, Util::BytesCompare32> collidingMessages;
    for (const AggregationInfo &info : infos) {
        std::set<const uint8_t*, Util::BytesCompare32> messagesLocal;
        for (auto &mapEntry : info.tree) {
            auto lookupEntry = messages.find(mapEntry.first);
            auto lookupEntryLocal = messagesLocal.find(mapEntry.first);
            if (lookupEntryLocal == messagesLocal.end() &&
                    lookupEntry != messages.end()) {
                collidingMessages.insert(mapEntry.first);
            }
            messages.insert(mapEntry.first);
            messagesLocal.insert(mapEntry.first);
        }
    }
    if (collidingMessages.empty()) {
        // If there are no colliding messages, combine without exponentiation
        return SimpleMergeInfos(infos);
    } else {
        // Otherwise, figure out with infos collide
        std::vector<AggregationInfo> collidingInfos;
        std::vector<AggregationInfo> nonCollidingInfos;
        for (const AggregationInfo &info : infos) {
            bool infoCollides = false;
            for (auto &mapEntry : info.tree) {
                auto lookupEntry = collidingMessages.find(mapEntry.first);
                if (lookupEntry != collidingMessages.end()) {
                    infoCollides = true;
                    collidingInfos.push_back(info);
                    break;
                }
            }
            if (!infoCollides) {
                nonCollidingInfos.push_back(info);
            }
        }
        // Combine the infos that collide securely (with exponentiation)
        AggregationInfo combined = SecureMergeInfos(collidingInfos);
        nonCollidingInfos.push_back(combined);

        // Do a simple combination of the rest (no exponentiation)
        return SimpleMergeInfos(nonCollidingInfos);
    }
}

AggregationInfo::AggregationInfo(const AggregationInfo& info) {
    InsertIntoTree(tree, info);
    SortIntoVectors(sortedMessageHashes, sortedPubKeys, tree);
}

void AggregationInfo::RemoveEntries(std::vector<uint8_t*> const &messages,
                                    std::vector<PublicKey> const &pubKeys) {
    if (messages.size() != pubKeys.size()) {
        throw std::string("Invalid entries");
    }
    // Erase the keys from the tree
    for (size_t i = 0; i < messages.size(); i++) {
        uint8_t entry[BLS::MESSAGE_HASH_LEN + PublicKey::PUBLIC_KEY_SIZE];
        std::memcpy(entry, messages[i], BLS::MESSAGE_HASH_LEN);
        pubKeys[i].Serialize(entry + BLS::MESSAGE_HASH_LEN);
        auto kv = tree.find(entry);
        const uint8_t* first = kv->first;
        const bn_t* second = kv->second;
        delete[] second;
        tree.erase(entry);
        delete[] first;
    }
    // Remove all elements from vectors, and regenerate them
    sortedMessageHashes.clear();
    sortedPubKeys.clear();
    SortIntoVectors(sortedMessageHashes, sortedPubKeys, tree);
}

void AggregationInfo::GetExponent(bn_t *result, const uint8_t* messageHash,
                                  const PublicKey &pubKey) const {
    uint8_t mapKey[BLS::MESSAGE_HASH_LEN +
            PublicKey::PUBLIC_KEY_SIZE];
    std::memcpy(mapKey, messageHash, BLS::MESSAGE_HASH_LEN);
    pubKey.Serialize(mapKey + BLS::MESSAGE_HASH_LEN);
    bn_copy(*result, *tree.at(mapKey));
}

std::vector<PublicKey> AggregationInfo::GetPubKeys() const {
    return sortedPubKeys;
}

std::vector<uint8_t*> AggregationInfo::GetMessageHashes() const {
    return sortedMessageHashes;
}

bool AggregationInfo::Empty() const {
    return tree.empty();
}

// Compares two aggregation infos by the following process:
// (A.msgHash || A.pk || A.exponent) < (B.msgHash || B.pk || B.exponent)
// for each element in their sorted trees.
bool operator<(AggregationInfo const&a, AggregationInfo const&b) {
    if (a.Empty() && b.Empty()) {
        return false;
    }
    bool lessThan = false;
    for (size_t i=0; i < a.sortedMessageHashes.size()
         || i < b.sortedMessageHashes.size(); i++) {
        // If we run out of elements, return
        if (a.sortedMessageHashes.size() == i) {
            lessThan = true;
            break;
        } else if (b.sortedMessageHashes.size() == i) {
            lessThan = false;
            break;
        }
        // Otherwise, generate the msgHash || pk element, and compare
        uint8_t bufferA[BLS::MESSAGE_HASH_LEN + PublicKey::PUBLIC_KEY_SIZE];
        uint8_t bufferB[BLS::MESSAGE_HASH_LEN + PublicKey::PUBLIC_KEY_SIZE];
        std::memcpy(bufferA, a.sortedMessageHashes[i], BLS::MESSAGE_HASH_LEN);
        std::memcpy(bufferB, b.sortedMessageHashes[i], BLS::MESSAGE_HASH_LEN);
        a.sortedPubKeys[i].Serialize(bufferA + BLS::MESSAGE_HASH_LEN);
        b.sortedPubKeys[i].Serialize(bufferB + BLS::MESSAGE_HASH_LEN);
        if (std::memcmp(bufferA, bufferB, BLS::MESSAGE_HASH_LEN
                        + PublicKey::PUBLIC_KEY_SIZE) < 0) {
            lessThan = true;
            break;
        } else if (std::memcmp(bufferA, bufferB, BLS::MESSAGE_HASH_LEN
                               + PublicKey::PUBLIC_KEY_SIZE) > 0) {
            lessThan = false;
            break;
        }
        // If they are equal, compare the exponents
        auto aExp = a.tree.find(bufferA);
        auto bExp = b.tree.find(bufferB);
        int cmpRes = bn_cmp(*aExp->second, *bExp->second);
        if (cmpRes == CMP_LT) {
            lessThan = true;
            break;
        } else if (cmpRes == CMP_GT) {
            lessThan = false;
            break;
        }
    }
    // If all comparisons are equal, false is returned.
    return lessThan;
}

bool operator==(AggregationInfo const&a, AggregationInfo const&b) {
    return !(a < b) && !(b < a);
}

bool operator!=(AggregationInfo const&a, AggregationInfo const&b) {
    return (a < b) || (b < a);
}

std::ostream &operator<<(std::ostream &os, AggregationInfo const &a) {
    for (auto &kv : a.tree) {
        os << Util::HexStr(kv.first, 80) << ".." << ":" << std::endl;
        uint8_t str[RELIC_BN_BYTES * 3 + 1];
        bn_write_bin(str, sizeof(str), *kv.second);
        os << Util::HexStr(str + RELIC_BN_BYTES * 3 + 1 - 5, 5)
           << std::endl;
    }
    return os;
}

AggregationInfo& AggregationInfo::operator=(const AggregationInfo &rhs) {
    Clear();
    InsertIntoTree(tree, rhs);
    SortIntoVectors(sortedMessageHashes, sortedPubKeys, tree);
    return *this;
}

void AggregationInfo::InsertIntoTree(AggregationInfo::AggregationTree &tree,
                                     const AggregationInfo& info) {
    for (auto &mapEntry : info.tree) {
        uint8_t* messageCopy = new uint8_t[BLS::MESSAGE_HASH_LEN
                + PublicKey::PUBLIC_KEY_SIZE];
        std::memcpy(messageCopy, mapEntry.first, BLS::MESSAGE_HASH_LEN
                + PublicKey::PUBLIC_KEY_SIZE);
        bn_t * exponent = new bn_t[1];
        bn_new(*exponent);
        bn_copy(*exponent, *mapEntry.second);
        bn_t ord;
        g1_get_ord(ord);
        bn_mod(*exponent, *exponent, ord);
        tree.insert(std::make_pair(messageCopy, exponent));
    }
}

// This method is used to keep an alternate copy of the tree
// keys (hashes and pks) in sorted order, for easy access.
// Note: these are sorted in mh + pk order.
void AggregationInfo::SortIntoVectors(std::vector<uint8_t*> &ms,
            std::vector<PublicKey> &pks,
            const AggregationInfo::AggregationTree &tree) {
    for (auto &kv : tree) {
        ms.push_back(kv.first);
    }
    sort(begin(ms), end(ms),
         Util::BytesCompare80());
    for (auto &m : ms) {
        pks.push_back(PublicKey::FromBytes(m + BLS::MESSAGE_HASH_LEN));
    }
}

// Simple merging, no exponentiation is performed
AggregationInfo AggregationInfo::SimpleMergeInfos(
        std::vector<AggregationInfo> const &infos) {
    std::set<PublicKey> pubKeysDedup;

    AggregationTree newTree;
    for (const AggregationInfo &info : infos) {
        InsertIntoTree(newTree, info);
    }
    std::vector<PublicKey> pks;
    std::vector<uint8_t*> messageHashes;
    SortIntoVectors(messageHashes, pks, newTree);

    return AggregationInfo(newTree, messageHashes, pks);
}

AggregationInfo AggregationInfo::SecureMergeInfos(
            std::vector<AggregationInfo> const &collidingInfosArg) {
    // Sort colliding Infos
    std::vector<size_t> sortedCollidingInfos(collidingInfosArg.size());
    size_t pkCount = 0;
    for (size_t i = 0; i < sortedCollidingInfos.size(); i++) {
        sortedCollidingInfos[i] = i;
        pkCount += collidingInfosArg[i].tree.size();
    }
    // Groups are sorted by message then pk then exponent
    // Each info object (and all of it's exponents) will be
    // exponentiated by one of the Ts
    std::sort(sortedCollidingInfos.begin(), sortedCollidingInfos.end(), [&collidingInfosArg](size_t a, size_t b) {
        return collidingInfosArg[a] < collidingInfosArg[b];
    });

    std::vector<uint8_t*> msgAndPks;
    std::vector<uint8_t*> serPks;
    std::vector<size_t> sortedKeys;
    msgAndPks.reserve(pkCount);
    serPks.reserve(pkCount);
    sortedKeys.reserve(pkCount);
    for (size_t i = 0; i < sortedCollidingInfos.size(); i++) {
        for (auto &mapEntry : collidingInfosArg[sortedCollidingInfos[i]].tree) {
            uint8_t *serPk = new uint8_t[PublicKey::PUBLIC_KEY_SIZE];
            memcpy(serPk, mapEntry.first + BLS::MESSAGE_HASH_LEN, PublicKey::PUBLIC_KEY_SIZE);

            msgAndPks.emplace_back(mapEntry.first);
            serPks.emplace_back(serPk);
            sortedKeys.emplace_back(sortedKeys.size());
        }
    }
    // Pks are sorted by message then pk
    std::sort(sortedKeys.begin(), sortedKeys.end(), [&msgAndPks](size_t a, size_t b) {
        return memcmp(msgAndPks[a], msgAndPks[b], BLS::MESSAGE_HASH_LEN + PublicKey::PUBLIC_KEY_SIZE) < 0;
    });

    // Calculate Ts
    // Each T is multiplied with an exponent in one of the collidingInfos
    bn_t* computedTs = new bn_t[sortedCollidingInfos.size()];
    for (size_t i = 0; i < sortedCollidingInfos.size(); i++) {
        bn_new(computedTs[i]);
    }
    BLS::HashPubKeys(computedTs, sortedCollidingInfos.size(), serPks, sortedKeys);

    bn_t ord;
    g1_get_ord(ord);

    // Merge the trees, multiplying by the Ts, and then adding
    // to total
    AggregationTree newTree;
    for (size_t i = 0; i < sortedCollidingInfos.size(); i++) {
        const AggregationInfo info = collidingInfosArg[sortedCollidingInfos[i]];
        for (auto &mapEntry : info.tree) {
            auto newMapEntry = newTree.find(mapEntry.first);
            if (newMapEntry == newTree.end()) {
                // This message & pk has not been included yet
                uint8_t* mapKeyCopy = new uint8_t[BLS::MESSAGE_HASH_LEN
                        + PublicKey::PUBLIC_KEY_SIZE];
                std::memcpy(mapKeyCopy, mapEntry.first, BLS::MESSAGE_HASH_LEN
                    + PublicKey::PUBLIC_KEY_SIZE);

                bn_t * exponent = new bn_t[1];
                bn_new(*exponent);
                bn_copy(*exponent, *mapEntry.second);
                bn_mul(*exponent, *exponent, computedTs[i]);
                bn_mod(*exponent, *exponent, ord);
                newTree.insert(std::make_pair(mapKeyCopy, exponent));
            } else {
                // This message & pk is already included. Multiply.
                bn_t tmp;
                bn_new(tmp);
                bn_copy(tmp, *mapEntry.second);
                bn_mul(tmp, tmp, computedTs[i]);
                bn_add(*newMapEntry->second, *newMapEntry->second, tmp);
                bn_mod(*newMapEntry->second, *newMapEntry->second, ord);
            }
        }
    }
    delete[] computedTs;
    std::vector<PublicKey> pks;
    std::vector<uint8_t*> messageHashes;
    SortIntoVectors(messageHashes, pks, newTree);

    for (auto p : serPks) {
        delete[] p;
    }

    return AggregationInfo(newTree, messageHashes, pks);
}

// Frees all memory
void AggregationInfo::Clear() {
    sortedMessageHashes.clear();
    sortedPubKeys.clear();
    if (!(tree.empty())) {
        for (auto &mapEntry : tree) {
            delete[] mapEntry.first;
            delete[] mapEntry.second;
        }
        tree.clear();
    }
}

AggregationInfo::AggregationInfo() {}

AggregationInfo::~AggregationInfo() {
    Clear();
}
} // end namespace bls
