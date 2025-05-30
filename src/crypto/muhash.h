// Copyright (c) 2017-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_MUHASH_H
#define BITCOIN_CRYPTO_MUHASH_H

#include <serialize.h>
#include <uint256.h>

#include <stdint.h>

class Num3072
{
private:
    void FullReduce();
    bool IsOverflow() const;
    Num3072 GetInverse() const;

public:
    static constexpr size_t BYTE_SIZE = 384;

#ifdef __SIZEOF_INT128__
    typedef unsigned __int128 double_limb_t;
    typedef signed __int128 signed_double_limb_t;
    typedef uint64_t limb_t;
    typedef int64_t signed_limb_t;
    static constexpr int LIMBS = 48;
    static constexpr int SIGNED_LIMBS = 50;
    static constexpr int LIMB_SIZE = 64;
    static constexpr int SIGNED_LIMB_SIZE = 62;
#else
    typedef uint64_t double_limb_t;
    typedef int64_t signed_double_limb_t;
    typedef uint32_t limb_t;
    typedef int32_t signed_limb_t;
    static constexpr int LIMBS = 96;
    static constexpr int SIGNED_LIMBS = 103;
    static constexpr int LIMB_SIZE = 32;
    static constexpr int SIGNED_LIMB_SIZE = 30;
#endif
    limb_t limbs[LIMBS];

    // Sanity check for Num3072 constants
    static_assert(LIMB_SIZE * LIMBS == 3072, "Num3072 isn't 3072 bits");
    static_assert(sizeof(double_limb_t) == sizeof(limb_t) * 2, "bad size for double_limb_t");
    static_assert(sizeof(limb_t) * 8 == LIMB_SIZE, "LIMB_SIZE is incorrect");
    static_assert(SIGNED_LIMB_SIZE * SIGNED_LIMBS > 3072, "SIGNED_LIMBS * SIGNED_LIMB_SIZE is too small");
    static_assert(3072 / SIGNED_LIMB_SIZE == SIGNED_LIMBS - 1, "Bit 3072 must land in top signed limb");

    // Hard coded values in MuHash3072 constructor and Finalize
    static_assert(sizeof(limb_t) == 4 || sizeof(limb_t) == 8, "bad size for limb_t");

    void Multiply(const Num3072& a);
    void Divide(const Num3072& a);
    void SetToOne();
    void ToBytes(unsigned char (&out)[BYTE_SIZE]);

    Num3072() { this->SetToOne(); };
    Num3072(const unsigned char (&data)[BYTE_SIZE]);

    SERIALIZE_METHODS(Num3072, obj)
    {
        for (auto& limb : obj.limbs) {
            READWRITE(limb);
        }
    }
};

/** A class representing MuHash sets
 *
 * MuHash is a hashing algorithm that supports adding set elements in any
 * order but also deleting in any order. As a result, it can maintain a
 * running sum for a set of data as a whole, and add/remove when data
 * is added to or removed from it. A downside of MuHash is that computing
 * an inverse is relatively expensive. This is solved by representing
 * the running value as a fraction, and multiplying added elements into
 * the numerator and removed elements into the denominator. Only when the
 * final hash is desired, a single modular inverse and multiplication is
 * needed to combine the two. The combination is also run on serialization
 * to allow for space-efficient storage on disk.
 *
 * As the update operations are also associative, H(a)+H(b)+H(c)+H(d) can
 * in fact be computed as (H(a)+H(b)) + (H(c)+H(d)). This implies that
 * all of this is perfectly parallellizable: each thread can process an
 * arbitrary subset of the update operations, allowing them to be
 * efficiently combined later.
 *
 * MuHash does not support checking if an element is already part of the
 * set. That is why this class does not enforce the use of a set as the
 * data it represents because there is no efficient way to do so.
 * It is possible to add elements more than once and also to remove
 * elements that have not been added before. However, this implementation
 * is intended to represent a set of elements.
 *
 * See also https://cseweb.ucsd.edu/~mihir/papers/inchash.pdf and
 * https://lists.linuxfoundation.org/pipermail/bitcoin-dev/2017-May/014337.html.
 */
class MuHash3072
{
private:
    Num3072 m_numerator;
    Num3072 m_denominator;

    Num3072 ToNum3072(std::span<const unsigned char> in);

public:
    /* The empty set. */
    MuHash3072() noexcept = default;

    /* A singleton with variable sized data in it. */
    explicit MuHash3072(std::span<const unsigned char> in) noexcept;

    /* Insert a single piece of data into the set. */
    MuHash3072& Insert(std::span<const unsigned char> in) noexcept;

    /* Remove a single piece of data from the set. */
    MuHash3072& Remove(std::span<const unsigned char> in) noexcept;

    /* Multiply (resulting in a hash for the union of the sets) */
    MuHash3072& operator*=(const MuHash3072& mul) noexcept;

    /* Divide (resulting in a hash for the difference of the sets) */
    MuHash3072& operator/=(const MuHash3072& div) noexcept;

    /* Finalize into a 32-byte hash. Does not change this object's value. */
    void Finalize(uint256& out) noexcept;

    SERIALIZE_METHODS(MuHash3072, obj)
    {
        READWRITE(obj.m_numerator);
        READWRITE(obj.m_denominator);
    }
};

#endif // BITCOIN_CRYPTO_MUHASH_H
