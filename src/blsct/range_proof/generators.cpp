// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/mcl/mcl.h>
#include <blsct/arith/mcl/mcl_init.h>
#include <blsct/building_block/generator_deriver.h>
#include <blsct/range_proof/generators.h>
#include <blsct/range_proof/setup.h>
#include <ctokens/tokenid.h>
#include <hash.h>
#include <tinyformat.h>
#include <util/strencodings.h>

template <typename T>
Elements<typename T::Point> range_proof::Generators<T>::GetGiSubset(const size_t& size) const
{
    return Gi.To(size);
}
template Elements<Mcl::Point> range_proof::Generators<Mcl>::GetGiSubset(const size_t&) const;

template <typename T>
Elements<typename T::Point> range_proof::Generators<T>::GetHiSubset(const size_t& size) const
{
    return Hi.To(size);
}
template Elements<Mcl::Point> range_proof::Generators<Mcl>::GetHiSubset(const size_t&) const;

template <typename T>
range_proof::GeneratorsFactory<T>::GeneratorsFactory()
{
    using Point = typename T::Point;

    std::lock_guard<std::mutex> lock(GeneratorsFactory<T>::m_init_mutex);
    if (GeneratorsFactory<T>::m_is_initialized) return;

    MclInit x;

    // H needs to be the nase point in order for the verification process to work
    m_H = Point::GetBasePoint();

    // Gi, Hi are derived from the base point and default TokenId seed
    Point base_point = Point::GetBasePoint();
    auto gi_hi_seed = TokenId();
    Point p = m_deriver.Derive(base_point, 0, gi_hi_seed);

    for (size_t i = 0; i < range_proof::Setup::max_input_value_vec_len; ++i) {
        const size_t base_index = i * 2;
        Point hi = m_deriver.Derive(p, base_index + 1, gi_hi_seed);
        Point gi = m_deriver.Derive(p, base_index + 2, gi_hi_seed);
        m_Hi.Add(hi);
        m_Gi.Add(gi);
    }

    // cache the point for later use
    m_G_cache.emplace(std::make_pair(gi_hi_seed, p));

    m_is_initialized = true;
}
template range_proof::GeneratorsFactory<Mcl>::GeneratorsFactory();

template <typename T>
range_proof::Generators<T> range_proof::GeneratorsFactory<T>::GetInstance(const Seed& seed) const
{
    using Point = typename T::Point;

    // if G for the given seed hasn't been created, create and cache it
    if (m_G_cache.count(seed) == 0) {
        const Point G = m_deriver.Derive(m_H, 0, seed);
        m_G_cache.emplace(seed, G);
    }
    Point G = m_G_cache[seed];

    Generators<T> gens(G, m_H, m_Gi, m_Hi);
    return gens;
}
template range_proof::Generators<Mcl> range_proof::GeneratorsFactory<Mcl>::GetInstance(const Seed&) const;

