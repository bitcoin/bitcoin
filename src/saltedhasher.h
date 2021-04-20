// Copyright (c) 2019-2020 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SALTEDHASHER_H
#define BITCOIN_SALTEDHASHER_H

#include <hash.h>
#include <uint256.h>

/** Helper classes for std::unordered_map and std::unordered_set hashing */

template<typename T> struct SaltedHasherImpl;

template<typename N>
struct SaltedHasherImpl<std::pair<uint256, N>>
{
    static std::size_t CalcHash(const std::pair<uint256, N>& v, uint64_t k0, uint64_t k1)
    {
        return SipHashUint256Extra(k0, k1, v.first, (uint32_t) v.second);
    }
};

template<typename N>
struct SaltedHasherImpl<std::pair<N, uint256>>
{
    static std::size_t CalcHash(const std::pair<N, uint256>& v, uint64_t k0, uint64_t k1)
    {
        return SipHashUint256Extra(k0, k1, v.second, (uint32_t) v.first);
    }
};

template<>
struct SaltedHasherImpl<uint256>
{
    static std::size_t CalcHash(const uint256& v, uint64_t k0, uint64_t k1)
    {
        return SipHashUint256(k0, k1, v);
    }
};

struct SaltedHasherBase
{
    /** Salt */
    const uint64_t k0, k1;

    SaltedHasherBase();
};

/* Allows each instance of unordered maps/sest to have their own salt */
template<typename T, typename S>
struct SaltedHasher
{
    S s;
    std::size_t operator()(const T& v) const
    {
        return SaltedHasherImpl<T>::CalcHash(v, s.k0, s.k1);
    }
};

/* Allows to use a static salt for all instances. The salt is a random value set at startup
 * (through static initialization)
 */
struct StaticSaltedHasher
{
    static SaltedHasherBase s;

    template<typename T>
    std::size_t operator()(const T& v) const
    {
        return SaltedHasherImpl<T>::CalcHash(v, s.k0, s.k1);
    }
};

#endif // BITCOIN_SALTEDHASHER_H
