// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/g1point.h>
#include <blsct/arith/generators.h>
#include <ctokens/tokenid.h>
#include <util/strencodings.h>
#include <tinyformat.h>

Generators::Generators(const TokenId token_id)
{
    if (!m_is_static_values_initialized) {
        throw std::runtime_error(strprintf("%s: Generators class not initialized", __func__));
    }
    // if H for the token_id hasn't been created, create it and store it to the cache
    if (Generators::m_H_cache.count(token_id) == 0) {
        const G1Point h_for_token_id = GetGenerator(Generators::m_G, 0, token_id);
        Generators::m_H_cache.emplace(token_id, h_for_token_id);
    }
    m_H = m_H_cache[token_id];
}

void Generators::Init(const size_t bit_size, const size_t max_value_vec_len)
{
    if (m_is_static_values_initialized) return;
    boost::lock_guard<boost::mutex> lock(Generators::m_init_mutex);

    const size_t num_generators = bit_size * max_value_vec_len;
    const TokenId default_token_id;
    const G1Point H = Generators::GetGenerator(G1Point::GetBasePoint(), 0, default_token_id);

    for (size_t i = 0; i < num_generators; ++i) {
        const size_t base_index = i * 2;
        G1Point hi = Generators::GetGenerator(H, base_index + 1, default_token_id);
        G1Point gi = Generators::GetGenerator(H, base_index + 2, default_token_id);
        m_Hi.Add(hi);
        m_Gi.Add(gi);
    }
    m_is_static_values_initialized = true;
}

G1Point Generators::GetGenerator(
    const G1Point& p,
    const size_t index,
    const TokenId& token_id)
{
    static const std::string salt("bulletproof");
    std::vector<uint8_t> serialized_p = p.GetVch();

    auto s =  token_id.token.ToString();
    std::string hash_preimage =
        HexStr(serialized_p) +
        salt +
        std::to_string(index) +
        token_id.token.ToString() +
        (token_id.subid != -1 ? "nft" + std::to_string(token_id.subid) : "");

    CHashWriter ss(SER_GETHASH, 0);
    ss << hash_preimage;
    auto hash = ss.GetHash();

    auto vec_hash = std::vector<uint8_t>(hash.begin(), hash.end());
    auto ret = G1Point::MapToG1(vec_hash);
    if (ret.IsUnity()) {
        throw std::runtime_error(strprintf("%s: generated G1Point is point at infinity", __func__));
    }
    return ret;
}

G1Point Generators::G() const
{
    return m_G;
}

G1Point Generators::H() const
{
    return m_H;
}

G1Points Generators::Gi() const
{
    return m_Gi;
}

G1Points Generators::Hi() const
{
    return m_Hi;
}
