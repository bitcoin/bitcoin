// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/mcl/mcl.h>
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
    using Points = Elements<Point>;

    std::lock_guard<std::mutex> lock(GeneratorsFactory<T>::m_init_mutex);
    if (GeneratorsFactory<T>::m_is_initialized) return;

    m_H = Point::GetBasePoint();
    Points Gi, Hi;
    m_Gi = Gi;
    m_Hi = Hi;

    const TokenId default_token_id;
    const Point default_G = m_deriver.Derive(Point::GetBasePoint(), 0, default_token_id);
    m_G_cache.insert(std::make_pair(default_token_id, default_G));

    for (size_t i = 0; i < range_proof::Setup::max_input_value_vec_len; ++i) {
        const size_t base_index = i * 2;
        Point hi = m_deriver.Derive(default_G, base_index + 1, default_token_id);
        Point gi = m_deriver.Derive(default_G, base_index + 2, default_token_id);
        m_Hi.Add(hi);
        m_Gi.Add(gi);
    }

    m_is_initialized = true;
}
template range_proof::GeneratorsFactory<Mcl>::GeneratorsFactory();

template <typename T>
range_proof::Generators<T> range_proof::GeneratorsFactory<T>::GetInstance(const TokenId& token_id)
{
    using Point = typename T::Point;

    // if G for the token_id hasn't been created, create and cache it
    if (GeneratorsFactory<T>::m_G_cache.count(token_id) == 0) {
        const Point G = m_deriver.Derive(GeneratorsFactory<T>::m_H, 0, token_id);
        GeneratorsFactory<T>::m_G_cache.emplace(token_id, G);
    }
    Point G = GeneratorsFactory<T>::m_G_cache[token_id];
    std::cout << __func__ << ": generator G=" << HexStr(G.GetVch()) << " for tokenId=" << token_id.ToString() << "\n";

    Generators<T> gens(m_H, G, m_Gi, m_Hi);
    return gens;
}
template range_proof::Generators<Mcl> range_proof::GeneratorsFactory<Mcl>::GetInstance(const TokenId&);
