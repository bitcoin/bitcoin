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

#include "signature.hpp"
#include "bls.hpp"

using std::string;
namespace bls {
InsecureSignature InsecureSignature::FromBytes(const uint8_t *data) {
    InsecureSignature sigObj = InsecureSignature();
    uint8_t uncompressed[SIGNATURE_SIZE + 1];
    std::memcpy(uncompressed + 1, data, SIGNATURE_SIZE);
    if (data[0] & 0x80) {
        uncompressed[0] = 0x03;   // Insert extra byte for Y=1
        uncompressed[1] &= 0x7f;  // Remove initial Y bit
    } else {
        uncompressed[0] = 0x02;   // Insert extra byte for Y=0
    }
    g2_read_bin(sigObj.sig, uncompressed, SIGNATURE_SIZE + 1);
    BLS::CheckRelicErrors();
    return sigObj;
}

InsecureSignature InsecureSignature::FromG2(const g2_t* element) {
    InsecureSignature sigObj = InsecureSignature();
    g2_copy(sigObj.sig, *(g2_t*)element);
    return sigObj;
}

InsecureSignature::InsecureSignature() {
    g2_set_infty(sig);
}

InsecureSignature::InsecureSignature(const InsecureSignature &signature) {
    g2_copy(sig, *(g2_t*)&signature.sig);
}

bool InsecureSignature::Verify(const std::vector<const uint8_t*>& hashes,
                                  const std::vector<PublicKey>& pubKeys) const {
    if (hashes.size() != pubKeys.size() || hashes.empty()) {
        throw std::string("hashes and pubKeys vectors must be of same size and non-empty");
    }

    g1_t *pubKeysNative = new g1_t[hashes.size() + 1];
    g2_t *mappedHashes = new g2_t[hashes.size() + 1];

    g2_copy(mappedHashes[0], *(g2_t*)&sig);
    g1_get_gen(pubKeysNative[0]);
    bn_t ordMinus1;
    bn_new(ordMinus1);
    g1_get_ord(ordMinus1);
    bn_sub_dig(ordMinus1, ordMinus1, 1);
    g1_mul(pubKeysNative[0], pubKeysNative[0], ordMinus1);

    for (size_t i = 0; i < hashes.size(); i++) {
        g2_map(mappedHashes[i + 1], hashes[i], BLS::MESSAGE_HASH_LEN, 0);
        g1_copy(pubKeysNative[i + 1], pubKeys[i].q);
    }

    bool result = VerifyNative(pubKeysNative, mappedHashes, hashes.size() + 1);

    delete[] pubKeysNative;
    delete[] mappedHashes;

    return result;
}

bool InsecureSignature::VerifyNative(
        g1_t* pubKeys,
        g2_t* mappedHashes,
        size_t len) {
    gt_t target, candidate;

    // Target = 1
    fp12_zero(target);
    fp_set_dig(target[0][0][0], 1);

    // prod e(pubkey[i], hash[i]) * e(-1 * g1, aggSig)
    // Performs pubKeys.size() pairings
    pc_map_sim(candidate, pubKeys, mappedHashes, len);

    // 1 =? prod e(pubkey[i], hash[i]) * e(g1, aggSig)
    if (gt_cmp(target, candidate) != CMP_EQ ||
        core_get()->code != STS_OK) {
        core_get()->code = STS_OK;
        return false;
    }
    BLS::CheckRelicErrors();
    return true;
}

InsecureSignature InsecureSignature::Aggregate(const std::vector<InsecureSignature>& sigs) {
    if (sigs.empty()) {
        throw std::string("sigs must not be empty");
    }
    InsecureSignature result = sigs[0];
    for (size_t i = 1; i < sigs.size(); i++) {
        g2_add(result.sig, result.sig, *(g2_t*)&sigs[i].sig);
    }
    return result;
}

InsecureSignature InsecureSignature::DivideBy(const std::vector<InsecureSignature>& sigs) const {
    if (sigs.empty()) {
        return *this;
    }

    InsecureSignature tmpAgg = Aggregate(sigs);
    InsecureSignature result(*this);
    g2_sub(result.sig, result.sig, tmpAgg.sig);
    return result;
}

InsecureSignature InsecureSignature::Exp(const bn_t n) const {
    InsecureSignature result(*this);
    g2_mul(result.sig, result.sig, n);
    return result;
}

void InsecureSignature::Serialize(uint8_t* buffer) const {
    CompressPoint(buffer, &sig);
}

std::vector<uint8_t> InsecureSignature::Serialize() const {
    std::vector<uint8_t> data(SIGNATURE_SIZE);
    Serialize(data.data());
    return data;
}

bool operator==(InsecureSignature const &a, InsecureSignature const &b) {
    return g2_cmp(*(g2_t*)&a.sig, *(g2_t*)b.sig) == CMP_EQ;
}

bool operator!=(InsecureSignature const &a, InsecureSignature const &b) {
    return !(a == b);
}

std::ostream &operator<<(std::ostream &os, InsecureSignature const &s) {
    uint8_t data[InsecureSignature::SIGNATURE_SIZE];
    s.Serialize(data);
    return os << Util::HexStr(data, InsecureSignature::SIGNATURE_SIZE);
}

InsecureSignature& InsecureSignature::operator=(const InsecureSignature &rhs) {
    g2_copy(sig, *(g2_t*)&rhs.sig);
    return *this;
}

void InsecureSignature::CompressPoint(uint8_t* result, const g2_t* point) {
    uint8_t buffer[InsecureSignature::SIGNATURE_SIZE + 1];
    g2_write_bin(buffer, InsecureSignature::SIGNATURE_SIZE + 1, *(g2_t*)point, 1);

    if (buffer[0] == 0x03) {
        buffer[1] |= 0x80;
    }
    std::memcpy(result, buffer + 1, SIGNATURE_SIZE);
}

///

Signature Signature::FromBytes(const uint8_t* data) {
    Signature result;
    result.sig = InsecureSignature::FromBytes(data);
    return result;
}

Signature Signature::FromBytes(const uint8_t *data, const AggregationInfo &info) {
    Signature ret = FromBytes(data);
    ret.SetAggregationInfo(info);
    return ret;
}

Signature Signature::FromG2(const g2_t* element) {
    Signature result;
    result.sig = InsecureSignature::FromG2(element);
    return result;
}

Signature Signature::FromG2(const g2_t* element, const AggregationInfo& info) {
    Signature ret = FromG2(element);
    ret.SetAggregationInfo(info);
    return ret;
}

Signature Signature::FromInsecureSig(const InsecureSignature& sig) {
    return FromG2(&sig.sig);
}

Signature Signature::FromInsecureSig(const InsecureSignature& sig, const AggregationInfo& info) {
    return FromG2(&sig.sig, info);
}

Signature::Signature(const Signature &_signature)
    : sig(_signature.sig),
      aggregationInfo(_signature.aggregationInfo) {
}

const AggregationInfo* Signature::GetAggregationInfo() const {
    return &aggregationInfo;
}

void Signature::SetAggregationInfo(
        const AggregationInfo &newAggregationInfo) {
    aggregationInfo = newAggregationInfo;
}

void Signature::Serialize(uint8_t* buffer) const {
    sig.Serialize(buffer);
}

std::vector<uint8_t> Signature::Serialize() const {
    return sig.Serialize();
}

InsecureSignature Signature::GetInsecureSig() const {
    return sig;
}

bool operator==(Signature const &a, Signature const &b) {
    return a.sig == b.sig;
}

bool operator!=(Signature const &a, Signature const &b) {
    return !(a == b);
}

std::ostream &operator<<(std::ostream &os, Signature const &s) {
    uint8_t data[InsecureSignature::SIGNATURE_SIZE];
    s.Serialize(data);
    return os << Util::HexStr(data, InsecureSignature::SIGNATURE_SIZE);
}

/*
 * This implementation of verify has several steps. First, it
 * reorganizes the pubkeys and messages into groups, where
 * each group corresponds to a message. Then, it checks if the
 * siganture has info on how it was aggregated. If so, we
 * exponentiate each pk based on the exponent in the AggregationInfo.
 * If not, we find public keys that share messages with others,
 * and aggregate all of these securely (with exponents.).
 * Finally, since each public key now corresponds to a unique
 * message (since we grouped them), we can verify using the
 * distinct verification procedure.
 */
bool Signature::Verify() const {
    if (GetAggregationInfo()->Empty()) {
        return false;
    }
    std::vector<PublicKey> pubKeys = GetAggregationInfo()
            ->GetPubKeys();
    std::vector<uint8_t*> messageHashes = GetAggregationInfo()
            ->GetMessageHashes();
    if (pubKeys.size() != messageHashes.size()) {
        return false;
    }
    // Group all of the messages that are idential, with the
    // pubkeys and signatures, the std::maps's key is the message hash
    std::map<uint8_t*, std::vector<PublicKey>,
            Util::BytesCompare32> hashToPubKeys;

    for (size_t i = 0; i < messageHashes.size(); i++) {
        auto pubKeyIter = hashToPubKeys.find(messageHashes[i]);
        if (pubKeyIter != hashToPubKeys.end()) {
            // Already one identical message, so push to vector
            pubKeyIter->second.push_back(pubKeys[i]);
        } else {
            // First time seeing this message, so create a vector
            std::vector<PublicKey> newPubKey = {pubKeys[i]};
            hashToPubKeys.insert(make_pair(messageHashes[i], newPubKey));
        }
    }

    // Aggregate pubkeys of identical messages
    std::vector<PublicKey> finalPubKeys;
    std::vector<const uint8_t*> finalMessageHashes;
    std::vector<const uint8_t*> collidingKeys;

    for (const auto &kv : hashToPubKeys) {
        PublicKey prod;
        std::map<uint8_t*, size_t, Util::BytesCompare<PublicKey::PUBLIC_KEY_SIZE>> dedupMap;
        for (size_t i = 0; i < kv.second.size(); i++) {
            const PublicKey& pk = kv.second[i];
            uint8_t *k = new uint8_t[PublicKey::PUBLIC_KEY_SIZE];
            pk.Serialize(k);
            dedupMap.emplace(k, i);
        }

        for (const auto &kv2 : dedupMap) {
            const PublicKey& pk = kv.second[kv2.second];

            bn_t exponent;
            bn_new(exponent);
            try {
                GetAggregationInfo()->GetExponent(&exponent, kv.first, pk);
            } catch (std::out_of_range) {
                for (auto &p : dedupMap) {
                    delete[] p.first;
                }
                return false;
            }
            prod = PublicKey::AggregateInsecure({prod, pk.Exp(exponent)});
        }
        finalPubKeys.push_back(prod);
        finalMessageHashes.push_back(kv.first);

        for (auto &p : dedupMap) {
            delete[] p.first;
        }
    }

    // Now we have all distinct messages, so we can verify
    return sig.Verify(finalMessageHashes, finalPubKeys);
}

Signature Signature::AggregateSigs(
        std::vector<Signature> const &sigs) {
    std::vector<std::vector<PublicKey> > pubKeys;
    std::vector<std::vector<uint8_t*> > messageHashes;

    // Extracts the public keys and messages from the aggregation info
    for (const Signature &sig : sigs) {
        const AggregationInfo &info = *sig.GetAggregationInfo();
        if (info.Empty()) {
            throw std::string("Signature must include aggregation info.");
        }
        std::vector<PublicKey> infoPubKeys = info.GetPubKeys();
        std::vector<uint8_t*> infoMessageHashes = info.GetMessageHashes();
        if (infoPubKeys.size() < 1 || infoMessageHashes.size() < 1) {
            throw std::string("AggregationInfo must have items");
        }
        pubKeys.push_back(infoPubKeys);
        std::vector<uint8_t*> currMessageHashes;
        for (const uint8_t* infoMessageHash : infoMessageHashes) {
            uint8_t* messageHash = new uint8_t[BLS::MESSAGE_HASH_LEN];
            std::memcpy(messageHash, infoMessageHash, BLS::MESSAGE_HASH_LEN);
            currMessageHashes.push_back(messageHash);
        }
        messageHashes.push_back(currMessageHashes);
    }

    if (sigs.size() != pubKeys.size()
        || pubKeys.size() != messageHashes.size()) {
        throw std::string("Lengths of vectors must match.");
    }
    for (size_t i = 0; i < messageHashes.size(); i++) {
        if (pubKeys[i].size() != messageHashes[i].size()) {
            throw std::string("Lengths of vectors must match.");
        }
    }
    Signature ret = AggregateSigsInternal(sigs, pubKeys,
                                             messageHashes);
    for (std::vector<uint8_t*> group : messageHashes) {
        for (const uint8_t* messageHash : group) {
            delete[] messageHash;
        }
    }
    return ret;
}

Signature Signature::AggregateSigsSecure(
        std::vector<Signature> const &sigs,
        std::vector<PublicKey> const &pubKeys,
        std::vector<uint8_t*> const &messageHashes) {
    if (sigs.size() != pubKeys.size() || sigs.size() != messageHashes.size()
        || sigs.size() < 1) {
        throw std::string("Must have atleast one signature, key, and message");
    }

    // Sort the public keys and signature by message + public key
    std::vector<uint8_t*> serPubKeys(pubKeys.size());
    std::vector<uint8_t*> sortKeys(pubKeys.size());
    std::vector<size_t> keysSorted(pubKeys.size());
    for (size_t i = 0; i < pubKeys.size(); i++) {
        serPubKeys[i] = new uint8_t[PublicKey::PUBLIC_KEY_SIZE];
        pubKeys[i].Serialize(serPubKeys[i]);

        uint8_t *sortKey = new uint8_t[BLS::MESSAGE_HASH_LEN + PublicKey::PUBLIC_KEY_SIZE];
        memcpy(sortKey, messageHashes[i], BLS::MESSAGE_HASH_LEN);
        memcpy(sortKey + BLS::MESSAGE_HASH_LEN, serPubKeys[i], PublicKey::PUBLIC_KEY_SIZE);

        sortKeys[i] = sortKey;
        keysSorted[i] = i;
    }

    std::sort(keysSorted.begin(), keysSorted.end(), [&sortKeys](size_t a, size_t b) {
        return memcmp(sortKeys[a], sortKeys[b], BLS::MESSAGE_HASH_LEN + PublicKey::PUBLIC_KEY_SIZE) < 0;
    });

    bn_t* computedTs = new bn_t[keysSorted.size()];
    for (size_t i = 0; i < keysSorted.size(); i++) {
        bn_new(computedTs[i]);
    }
    BLS::HashPubKeys(computedTs, keysSorted.size(), serPubKeys, keysSorted);

    // Raise all signatures to power of the corresponding t's and aggregate the results into aggSig
    std::vector<InsecureSignature> expSigs;
    expSigs.reserve(keysSorted.size());
    for (size_t i = 0; i < keysSorted.size(); i++) {
        auto& s = sigs[keysSorted[i]].sig;
        expSigs.emplace_back(s.Exp(computedTs[i]));
    }
    InsecureSignature aggSig = InsecureSignature::Aggregate(expSigs);

    delete[] computedTs;
    for (auto p : serPubKeys) {
        delete[] p;
    }
    for (auto p : sortKeys) {
        delete[] p;
    }

    Signature ret = Signature::FromInsecureSig(aggSig);
    BLS::CheckRelicErrors();
    return ret;
}

Signature Signature::AggregateSigsInternal(
        std::vector<Signature> const &sigs,
        std::vector<std::vector<PublicKey> > const &pubKeys,
        std::vector<std::vector<uint8_t*> > const &messageHashes) {
    if (sigs.size() != pubKeys.size()
        || pubKeys.size() != messageHashes.size()) {
        throw std::string("Lengths of std::vectors must match.");
    }
    for (size_t i = 0; i < messageHashes.size(); i++) {
        if (pubKeys[i].size() != messageHashes[i].size()) {
            throw std::string("Lengths of std::vectors must match.");
        }
    }

    // Find colliding vectors, save colliding messages
    std::set<const uint8_t*, Util::BytesCompare32> messagesSet;
    std::set<const uint8_t*, Util::BytesCompare32> collidingMessagesSet;
    for (auto &msgVector : messageHashes) {
        std::set<const uint8_t*, Util::BytesCompare32> messagesSetLocal;
        for (auto &msg : msgVector) {
            auto lookupEntry = messagesSet.find(msg);
            auto lookupEntryLocal = messagesSetLocal.find(msg);
            if (lookupEntryLocal == messagesSetLocal.end() &&
                lookupEntry != messagesSet.end()) {
                collidingMessagesSet.insert(msg);
            }
            messagesSet.insert(msg);
            messagesSetLocal.insert(msg);
        }
    }
    if (collidingMessagesSet.empty()) {
        // There are no colliding messages between the groups, so we
        // will just aggregate them all simply. Note that we assume
        // that every group is a valid aggregate signature. If an invalid
        // or insecure signature is given, and invalid signature will
        // be created. We don't verify for performance reasons.
        Signature ret = AggregateSigsSimple(sigs);
        std::vector<AggregationInfo> infos;
        for (const Signature &sig : sigs) {
            infos.push_back(*sig.GetAggregationInfo());
        }
        ret.SetAggregationInfo(AggregationInfo::MergeInfos(infos));
        return ret;
    }

    // There are groups that share messages, therefore we need
    // to use a secure form of aggregation. First we find which
    // groups collide, and securely aggregate these. Then, we
    // use simple aggregation at the end.
    std::vector<Signature > collidingSigs;
    std::vector<Signature> nonCollidingSigs;
    std::vector<std::vector<uint8_t*> > collidingMessageHashes;
    std::vector<std::vector<PublicKey> > collidingPks;

    for (size_t i = 0; i < sigs.size(); i++) {
        bool groupCollides = false;
        for (const uint8_t* msg : messageHashes[i]) {
            auto lookupEntry = collidingMessagesSet.find(msg);
            if (lookupEntry != collidingMessagesSet.end()) {
                groupCollides = true;
                collidingSigs.push_back(sigs[i]);
                collidingMessageHashes.push_back(messageHashes[i]);
                collidingPks.push_back(pubKeys[i]);
                break;
            }
        }
        if (!groupCollides) {
            nonCollidingSigs.push_back(sigs[i]);
        }
    }

    // Sort signatures by aggInfo
    std::vector<size_t> sigsSorted(collidingSigs.size());
    for (size_t i = 0; i < sigsSorted.size(); i++) {
        sigsSorted[i] = i;
    }
    std::sort(sigsSorted.begin(), sigsSorted.end(), [&collidingSigs](size_t a, size_t b) {
        return *collidingSigs[a].GetAggregationInfo() < *collidingSigs[b].GetAggregationInfo();
    });

    std::vector<uint8_t*> serPubKeys;
    std::vector<uint8_t*> sortKeys;
    std::vector<size_t> sortKeysSorted;
    size_t sortKeysCount = 0;
    for (size_t i = 0; i < collidingPks.size(); i++) {
        sortKeysCount += collidingPks[i].size();
    }
    sortKeys.reserve(sortKeysCount);
    sortKeysSorted.reserve(sortKeysCount);
    for (size_t i = 0; i < collidingPks.size(); i++) {
        for (size_t j = 0; j < collidingPks[i].size(); j++) {
            uint8_t *serPk = new uint8_t[PublicKey::PUBLIC_KEY_SIZE];
            uint8_t *sortKey = new uint8_t[BLS::MESSAGE_HASH_LEN + PublicKey::PUBLIC_KEY_SIZE];
            collidingPks[i][j].Serialize(serPk);
            std::memcpy(sortKey, collidingMessageHashes[i][j], BLS::MESSAGE_HASH_LEN);
            std::memcpy(sortKey + BLS::MESSAGE_HASH_LEN, serPk, PublicKey::PUBLIC_KEY_SIZE);
            serPubKeys.emplace_back(serPk);
            sortKeysSorted.emplace_back(sortKeys.size());
            sortKeys.emplace_back(sortKey);
        }
    }
    // Sort everything according to message || pubkey
    std::sort(sortKeysSorted.begin(), sortKeysSorted.end(), [&sortKeys](size_t a, size_t b) {
        return memcmp(sortKeys[a], sortKeys[b], BLS::MESSAGE_HASH_LEN + PublicKey::PUBLIC_KEY_SIZE) < 0;
    });

    std::vector<PublicKey> pubKeysSorted;
    for (size_t i = 0; i < sortKeysSorted.size(); i++) {
        const uint8_t *sortKey = sortKeys[sortKeysSorted[i]];
        pubKeysSorted.push_back(PublicKey::FromBytes(sortKey
                                                        + BLS::MESSAGE_HASH_LEN));
    }
    bn_t* computedTs = new bn_t[sigsSorted.size()];
    for (size_t i = 0; i < sigsSorted.size(); i++) {
        bn_new(computedTs[i]);
    }
    BLS::HashPubKeys(computedTs, sigsSorted.size(), serPubKeys, sortKeysSorted);

    // Raise all signatures to power of the corresponding t's and aggregate the results into aggSig
    // Also accumulates aggregation info for each signature
    std::vector<AggregationInfo> infos;
    std::vector<InsecureSignature> expSigs;
    infos.reserve(sigsSorted.size());
    expSigs.reserve(sigsSorted.size());
    for (size_t i = 0; i < sigsSorted.size(); i++) {
        auto& s = collidingSigs[sigsSorted[i]];
        expSigs.emplace_back(s.sig.Exp(computedTs[i]));
        infos.emplace_back(*s.GetAggregationInfo());
    }

    // Also collect all non-colliding signatures for aggregation
    // These don't need exponentiation
    for (const Signature &nonColliding : nonCollidingSigs) {
        expSigs.emplace_back(nonColliding.sig);
        infos.emplace_back(*nonColliding.GetAggregationInfo());
    }

    InsecureSignature aggSig = InsecureSignature::Aggregate(expSigs);
    Signature ret = Signature::FromInsecureSig(aggSig);

    // Merge the aggregation infos, which will be combined in an
    // identical way as above.
    ret.SetAggregationInfo(AggregationInfo::MergeInfos(infos));

    delete[] computedTs;

    for (auto p : serPubKeys) {
        delete[] p;
    }
    for (auto p : sortKeys) {
        delete[] p;
    }

    return ret;
}

Signature Signature::AggregateSigsSimple(std::vector<Signature> const &sigs) {
    if (sigs.size() < 1) {
        throw std::string("Must have atleast one signatures and key");
    }
    if (sigs.size() == 1) {
        return sigs[0];
    }

    // Multiplies the signatures together (relic uses additive group operation)
    std::vector<InsecureSignature> sigs2;
    sigs2.reserve(sigs.size());
    for (const Signature &sig : sigs) {
        sigs2.emplace_back(sig.sig);
    }
    InsecureSignature aggSig = InsecureSignature::Aggregate(sigs2);
    Signature ret = Signature::FromInsecureSig(aggSig);
    BLS::CheckRelicErrors();
    return ret;
}

Signature Signature::DivideBy(std::vector<Signature> const &divisorSigs) const {
    bn_t ord;
    g2_get_ord(ord);

    std::vector<uint8_t*> messageHashesToRemove;
    std::vector<PublicKey> pubKeysToRemove;

    std::vector<InsecureSignature> expSigs;
    expSigs.reserve(divisorSigs.size());
    for (const Signature &divisorSig : divisorSigs) {
        std::vector<PublicKey> pks = divisorSig.GetAggregationInfo()
                ->GetPubKeys();
        std::vector<uint8_t*> messageHashes = divisorSig.GetAggregationInfo()
                ->GetMessageHashes();
        if (pks.size() != messageHashes.size()) {
            throw string("Invalid aggregation info.");
        }
        bn_t quotient;
        for (size_t i = 0; i < pks.size(); i++) {
            bn_t divisor;
            bn_new(divisor);
            divisorSig.GetAggregationInfo()->GetExponent(&divisor,
                    messageHashes[i],
                    pks[i]);
            bn_t dividend;
            bn_new(dividend);
            try {
                aggregationInfo.GetExponent(&dividend, messageHashes[i],
                                            pks[i]);
            } catch (std::out_of_range e) {
                throw string("Signature is not a subset.");
            }

            bn_t inverted;
            fp_inv_exgcd_bn(inverted, divisor, ord);

            if (i == 0) {
                bn_mul(quotient, dividend, inverted);
                bn_mod(quotient, quotient, ord);
            } else {
                bn_t newQuotient;
                bn_mul(newQuotient, dividend, inverted);
                bn_mod(newQuotient, newQuotient, ord);

                if (bn_cmp(quotient, newQuotient) != CMP_EQ) {
                    throw string("Cannot divide by aggregate signature,"
                                 "msg/pk pairs are not unique");
                }
            }
            messageHashesToRemove.push_back(messageHashes[i]);
            pubKeysToRemove.push_back(pks[i]);
        }
        expSigs.emplace_back(divisorSig.sig.Exp(quotient));
    }

    InsecureSignature prod = sig.DivideBy(expSigs);
    Signature result = Signature::FromInsecureSig(prod, aggregationInfo);
    result.aggregationInfo.RemoveEntries(messageHashesToRemove, pubKeysToRemove);

    return result;
}
} // end namespace bls

