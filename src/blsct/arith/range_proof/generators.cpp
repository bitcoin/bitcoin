// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/g1point.h>
#include <blsct/arith/range_proof/config.h>
#include <blsct/arith/range_proof/generators.h>
#include <ctokens/tokenid.h>
#include <util/strencodings.h>
#include <tinyformat.h>

void Generators::Init()
{
    printf("INIT CALLED\n");
    if (m_is_static_values_initialized) return;
    boost::lock_guard<boost::mutex> lock(Generators::m_init_mutex);

    MclInitializer::Init();
    G1Point::Init();

    m_G = G1Point::GetBasePoint();

    const TokenId default_token_id;
    const G1Point H = Generators::GetGenerator(G1Point::GetBasePoint(), 0, default_token_id);

    for (size_t i = 0; i < Config::m_max_input_value_vec_len; ++i) {
        const size_t base_index = i * 2;
        G1Point hi = Generators::GetGenerator(H, base_index + 1, default_token_id);
        G1Point gi = Generators::GetGenerator(H, base_index + 2, default_token_id);
        m_Hi.Add(hi);
        m_Gi.Add(gi);
    }
    m_is_static_values_initialized = true;
}

Generators::Generators()
{
    if (m_is_static_values_initialized) return;
    Generators::Init();
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
        (token_id.subid == std::numeric_limits<uint64_t>::max() ? "" : "nft" + std::to_string(token_id.subid));

    CHashWriter ss(SER_GETHASH, 0);
    ss << hash_preimage;
    auto hash = ss.GetHash();

    auto vec_hash = std::vector<uint8_t>(hash.begin(), hash.end());
    auto ret = G1Point::MapToG1(vec_hash);
    if (ret.IsUnity()) {
        throw std::runtime_error(strprintf("%s: generated G1Point is the point at infinity. change index or token_id to get a valid point", __func__));
    }
    return ret;
}

G1Point Generators::H(const TokenId& token_id)
{
    // if H for the token_id hasn't been created, create it and store it to the cache
    if (Generators::m_H_cache.count(token_id) == 0) {
        const G1Point H = GetGenerator(Generators::m_G, 0, token_id);
        Generators::m_H_cache.emplace(token_id, H);
    }
    return m_H_cache[token_id];
}

G1Point Generators::G() const
{
    return m_G;
}

G1Points Generators::Gi() const
{
    return m_Gi;
}

G1Points Generators::Hi() const
{
    return m_Hi;
}
