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

#ifndef SRC_BLSSIGNATURE_HPP_
#define SRC_BLSSIGNATURE_HPP_

#include <iostream>
#include <vector>

#include "relic_conf.h"

#if defined GMP && ARITH == GMP
#include <gmp.h>
#endif

#include "util.hpp"
#include "aggregationinfo.hpp"
namespace bls {
/**
 * An insecure BLS signature.
 * A Signature is a group element of g2
 * Aggregation of these signatures is not secure on it's own, use Signature instead
 */
class InsecureSignature {
 friend class Signature;
 template <typename T> friend struct PolyOps;
 public:
    static const size_t SIGNATURE_SIZE = 96;

    // Initializes from serialized byte array/
    static InsecureSignature FromBytes(const uint8_t *data);

    // Initializes from native relic g2 element/
    static InsecureSignature FromG2(const g2_t* element);

    // Copy constructor. Deep copies contents.
    InsecureSignature(const InsecureSignature &signature);

    // This verification method is insecure in regard to the rogue public key attack
    bool Verify(const std::vector<const uint8_t*>& hashes, const std::vector<PublicKey>& pubKeys) const;

    // Insecurely aggregates signatures
    static InsecureSignature Aggregate(const std::vector<InsecureSignature>& sigs);

    // Insecurely divides signatures
    InsecureSignature DivideBy(const std::vector<InsecureSignature>& sigs) const;

    // Serializes ONLY the 96 byte public key. It does not serialize
    // the aggregation info.
    void Serialize(uint8_t* buffer) const;
    std::vector<uint8_t> Serialize() const;

    friend bool operator==(InsecureSignature const &a, InsecureSignature const &b);
    friend bool operator!=(InsecureSignature const &a, InsecureSignature const &b);
    friend std::ostream &operator<<(std::ostream &os, InsecureSignature const &s);
    InsecureSignature& operator=(const InsecureSignature& rhs);

 public:
    // Prevent public construction, force static method
    InsecureSignature();
 private:

    // Exponentiate signature with n
    InsecureSignature Exp(const bn_t n) const;

    static void CompressPoint(uint8_t* result, const g2_t* point);

    // Performs multipairing and checks that everything matches. This is an
    // internal method, only called from Verify. It should not be used
    // anywhere else.
    static bool VerifyNative(
            g1_t* pubKeys,
            g2_t* mappedHashes,
            size_t len);

 private:
    // Signature group element
    g2_t sig;
};

/**
 * An encapsulated signature.
 * A Signature is composed of two things:
 *     1. 96 byte group element of g2
 *     2. AggregationInfo object, which describes how the signature was
 *        generated, and how it should be verified.
 */
class Signature {
 public:
    static const size_t SIGNATURE_SIZE = InsecureSignature::SIGNATURE_SIZE;

    // Initializes from serialized byte array/
    static Signature FromBytes(const uint8_t *data);

    // Initializes from bytes with AggregationInfo/
    static Signature FromBytes(const uint8_t *data, const AggregationInfo &info);

    // Initializes from native relic g2 element/
    static Signature FromG2(const g2_t* element);

    // Initializes from native relic g2 element with AggregationInfo/
    static Signature FromG2(const g2_t* element, const AggregationInfo &info);

    // Initializes from insecure signature/
    static Signature FromInsecureSig(const InsecureSignature& sig);

    // Initializes from insecure signature with AggregationInfo/
    static Signature FromInsecureSig(const InsecureSignature& sig, const AggregationInfo &info);

    // Copy constructor. Deep copies contents.
    Signature(const Signature &signature);

    // Verifies a single or aggregate signature.
    // Performs two pairing operations, sig must contain information on
    // how aggregation was performed (AggregationInfo). The Aggregation
    // Info contains all the public keys and messages required.
    bool Verify() const;

    // Securely aggregates many signatures on messages, some of
    // which may be identical. The signature can then be verified
    // using VerifyAggregate. The returned signature contains
    // information on how the aggregation was done (AggragationInfo).
    static Signature AggregateSigs(
            std::vector<Signature> const &sigs);

    // Divides the aggregate signature (this) by a list of signatures.
    // These divisors can be single or aggregate signatures, but all
    // msg/pk pairs in these signatures must be distinct and unique.
    Signature DivideBy(std::vector<Signature> const &divisorSigs) const;

    // Gets the aggregation info on this signature.
    const AggregationInfo* GetAggregationInfo() const;

    // Sets the aggregation information on this signature, which
    // describes how this signature was generated, and how it should
    // be verified.
    void SetAggregationInfo(const AggregationInfo &newAggregationInfo);

    // Serializes ONLY the 96 byte public key. It does not serialize
    // the aggregation info.
    void Serialize(uint8_t* buffer) const;
    std::vector<uint8_t> Serialize() const;

    InsecureSignature GetInsecureSig() const;

    friend bool operator==(Signature const &a, Signature const &b);
    friend bool operator!=(Signature const &a, Signature const &b);
    friend std::ostream &operator<<(std::ostream &os, Signature const &s);

 private:
    // Prevent public construction, force static method
    Signature() {}

    // Aggregates many signatures using the secure aggregation method.
    // Performs ~ n * 256 g2 operations.
    static Signature AggregateSigsSecure(
            std::vector<Signature> const &sigs,
            std::vector<PublicKey> const &pubKeys,
            std::vector<uint8_t*> const &messageHashes);

    // Internal methods
    static Signature AggregateSigsInternal(
            std::vector<Signature> const &sigs,
            std::vector<std::vector<PublicKey> > const &pubKeys,
            std::vector<std::vector<uint8_t*> > const &messageHashes);

    // Efficiently aggregates many signatures using the simple aggregation
    // method. Performs only n g2 operations.
    static Signature AggregateSigsSimple(
            std::vector<Signature> const &sigs);

 private:
    // internal signature
    InsecureSignature sig;

    // Optional info about how this was aggregated
    AggregationInfo aggregationInfo;
};
} // end namespace bls

#endif  // SRC_BLSSIGNATURE_HPP_
