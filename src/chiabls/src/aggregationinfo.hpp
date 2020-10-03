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

#ifndef SRC_AGGREGATIONINFO_HPP_
#define SRC_AGGREGATIONINFO_HPP_

#include <map>
#include <vector>
#include "publickey.hpp"
#include "util.hpp"
namespace bls {
/**
 * Represents information about how aggregation was performed,
 * or how a signature was generated (pks, messageHashes, etc).
 * The AggregationTree is a map from messageHash, pk -> exponent.
 * The exponent is the number that the public key needs to be
 * raised to, and the messageHash is the message that was signed,
 * and it's signature raised to that exponent. The flat
 * representation allows for simple and efficient lookup of a
 * given public key when verifying an aggregate signature.
 *
 * Invariant: always maintains sortedMessageHashes and sortedPubKeys
 * for efficiency. This data is equivalent to the keys in tree.
 */
class AggregationInfo {
 public:
    // Creates a base aggregation info object.
    static AggregationInfo FromMsgHash(const PublicKey &pk,
                                       const uint8_t* messageHash);

    static AggregationInfo FromMsg(const PublicKey &pk,
                                   const uint8_t* message,
                                   size_t len);

    static AggregationInfo FromVectors(
                std::vector<PublicKey> const &pubKeys,
                std::vector<uint8_t*> const &messageHashes,
                std::vector<bn_t*> const &exponents);

    // Merge two AggregationInfo objects into one.
    static AggregationInfo MergeInfos(std::vector<AggregationInfo>
                                      const &infos);

    // Copy constructor, deep copies data.
    AggregationInfo(const AggregationInfo& info);

    // Removes the messages and pubkeys from the tree
    void RemoveEntries(std::vector<uint8_t*> const &messages,
                       std::vector<PublicKey> const &pubKeys);

    // Public accessors
    void GetExponent(bn_t *result, const uint8_t* messageHash,
                     const PublicKey &pubkey) const;
    std::vector<PublicKey> GetPubKeys() const;
    std::vector<uint8_t*> GetMessageHashes() const;
    bool Empty() const;

    // Overloaded operators.
    friend bool operator<(AggregationInfo const &a, AggregationInfo const &b);
    friend bool operator==(AggregationInfo const &a, AggregationInfo const &b);
    friend bool operator!=(AggregationInfo const &a, AggregationInfo const &b);
    friend std::ostream &operator<<(std::ostream &os, AggregationInfo const &a);
    AggregationInfo& operator=(const AggregationInfo& rhs);

    AggregationInfo();
    ~AggregationInfo();

 private:
    // This is the data structure that maps messages (32) and
    // public keys (48) to exponents (bn_t*).
    typedef std::map<uint8_t*, bn_t*,
                     Util::BytesCompare80> AggregationTree;

    explicit AggregationInfo(const AggregationTree& tr,
                             std::vector<uint8_t*> ms,
                             std::vector<PublicKey> pks)
         : tree(tr),
           sortedMessageHashes(ms),
           sortedPubKeys(pks) {}

    static void InsertIntoTree(AggregationTree &tree,
                               const AggregationInfo& info);
    static void SortIntoVectors(std::vector<uint8_t*> &ms,
                                std::vector<PublicKey> &pks,
                                const AggregationTree &tree);
    static AggregationInfo SimpleMergeInfos(
            std::vector<AggregationInfo> const &infos);
    static AggregationInfo SecureMergeInfos(
            std::vector<AggregationInfo> const &infos);
    void Clear();

    AggregationTree tree;
    std::vector<uint8_t*> sortedMessageHashes;
    std::vector<PublicKey> sortedPubKeys;
};
} // end namespace bls

#endif  // SRC_AGGREGATIONINFO_HPP_
