#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <mw/common/Macros.h>
#include <mw/common/Traits.h>
#include <mw/models/crypto/BigInteger.h>
#include <boost/functional/hash.hpp>

MW_NAMESPACE

using Hash = BigInt<32>;

END_NAMESPACE

namespace std
{
    template<>
    struct hash<mw::Hash>
    {
        size_t operator()(const mw::Hash& hash) const
        {
            return boost::hash_value(hash.vec());
        }
    };
}

class Hashes
{
public:
    template <class T, typename SFINAE = typename std::enable_if_t<std::is_base_of<Traits::IHashable, T>::value>>
    static std::vector<mw::Hash> From(const std::vector<T>& vec_hashable) noexcept
    {
        std::vector<mw::Hash> hashes;
        std::transform(
            vec_hashable.cbegin(), vec_hashable.cend(),
            std::back_inserter(hashes),
            [](const T& hashable) { return hashable.GetHash(); });

        return hashes;
    }
};