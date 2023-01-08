// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/mcl/mcl_g1point.h>
#include <blsct/arith/mcl/mcl.h>
#include <blsct/range_proof/config.h>
#include <blsct/range_proof/generators.h>
#include <ctokens/tokenid.h>
#include <hash.h>
#include <util/strencodings.h>
#include <tinyformat.h>

template <typename T>
Elements<typename T::Point> Generators<T>::GetGiSubset(const size_t& size) const
{
    return Gi.get().To(size);
}
template Elements<Mcl::Point> Generators<Mcl>::GetGiSubset(const size_t&) const;

template <typename T>
Elements<typename T::Point> Generators<T>::GetHiSubset(const size_t& size) const
{
    return Hi.get().To(size);
}
template Elements<Mcl::Point> Generators<Mcl>::GetHiSubset(const size_t&) const;

template <typename T>
GeneratorsFactory<T>::GeneratorsFactory()
{
    using Point = typename T::Point;
    using Points = Elements<Point>;

    boost::lock_guard<boost::mutex> lock(GeneratorsFactory<T>::m_init_mutex);
    if (GeneratorsFactory<T>::m_is_initialized) return;

    T::Initializer::Init();
    T::Point::Init();

    m_H = Point::GetBasePoint();
    Points Gi, Hi;
    m_Gi = Gi;
    m_Hi = Hi;

    const TokenId default_token_id;
    const Point default_G = DeriveGenerator(Point::GetBasePoint(), 0, default_token_id);
    m_G_cache.insert(std::make_pair(default_token_id, default_G));

    for (size_t i = 0; i < Config::m_max_input_value_vec_len; ++i) {
        const size_t base_index = i * 2;
        Point hi = DeriveGenerator(default_G, base_index + 1, default_token_id);
        Point gi = DeriveGenerator(default_G, base_index + 2, default_token_id);
        m_Hi.value().Add(hi);
        m_Gi.value().Add(gi);
    }

    m_is_initialized = true;
}
template GeneratorsFactory<Mcl>::GeneratorsFactory();

template <typename T>
typename T::Point GeneratorsFactory<T>::DeriveGenerator(
    const typename T::Point& p,
    const size_t index,
    const TokenId& token_id)
{
    static const std::string salt("bulletproof");
    std::vector<uint8_t> serialized_p = p.GetVch();

    auto num_to_str = [](auto n) {
        std::ostringstream os;
        os << n;
        return os.str();
    };
    std::string hash_preimage =
        HexStr(serialized_p) +
        salt +
        num_to_str(index) +
        token_id.token.ToString() +
        (token_id.subid == std::numeric_limits<uint64_t>::max() ? "" : "nft" + num_to_str(token_id.subid));

    CHashWriter ss(SER_GETHASH, 0);
    ss << hash_preimage;
    auto hash = ss.GetHash();

    auto vec_hash = std::vector<uint8_t>(hash.begin(), hash.end());
    auto ret = Point::MapToG1(vec_hash);
    if (ret.IsUnity()) {
        throw std::runtime_error(strprintf(
            "%s: Generated G1Point is the point at infinity. Try changing parameters", __func__));
    }
    return ret;
}
template Mcl::Point GeneratorsFactory<Mcl>::DeriveGenerator(const Mcl::Point&, const size_t, const TokenId&);

template <typename T>
Generators<T> GeneratorsFactory<T>::GetInstance(const TokenId& token_id)
{
    using Point = typename T::Point;

    // if G for the token_id hasn't been created, create and cache it
    if (GeneratorsFactory<T>::m_G_cache.count(token_id) == 0) {
        const Point G = DeriveGenerator(GeneratorsFactory<T>::m_H.value(), 0, token_id);
        GeneratorsFactory<T>::m_G_cache.emplace(token_id, G);
    }
    Point G = GeneratorsFactory<T>::m_G_cache[token_id];

    Generators<T> gens(m_H.value(), G, m_Gi.value(), m_Hi.value());
    return gens;
}
template Generators<Mcl> GeneratorsFactory<Mcl>::GetInstance(const TokenId&);
