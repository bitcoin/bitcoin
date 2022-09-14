// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/g1point.h>
#include <blsct/arith/range_proof/config.h>
#include <blsct/arith/range_proof/generators.h>
#include <ctokens/tokenid.h>
#include <util/strencodings.h>
#include <tinyformat.h>

GeneratorsFactory::GeneratorsFactory()
{
    boost::lock_guard<boost::mutex> lock(GeneratorsFactory::m_init_mutex);
    if (GeneratorsFactory::m_is_initialized) return;

    MclInitializer::Init();
    G1Point::Init();

    m_G = G1Point::GetBasePoint();
    G1Points Gi, Hi;
    m_Gi = Gi;
    m_Hi = Hi;

    const TokenId default_token_id;
    const G1Point H = GetGenerator(G1Point::GetBasePoint(), 0, default_token_id);

    for (size_t i = 0; i < Config::m_max_input_value_vec_len; ++i) {
        const size_t base_index = i * 2;
        G1Point hi = GetGenerator(H, base_index + 1, default_token_id);
        G1Point gi = GetGenerator(H, base_index + 2, default_token_id);
        m_Hi.value().Add(hi);
        m_Gi.value().Add(gi);
    }

    m_is_initialized = true;
}

G1Point GeneratorsFactory::GetGenerator(
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
        throw std::runtime_error(strprintf("%s: generated G1Point is the point at infinity. try changing index and/or token_id to get a valid point", __func__));
    }
    return ret;
}

Generators GeneratorsFactory::GetInstance(const TokenId& token_id)
{
    // if H for the token_id hasn't been created, create it and store it to the cache
    if (GeneratorsFactory::m_H_cache.count(token_id) == 0) {
        const G1Point H = GetGenerator(GeneratorsFactory::m_G.value(), 0, token_id);
        GeneratorsFactory::m_H_cache.emplace(token_id, H);
    }
    G1Point H = GeneratorsFactory::m_H_cache[token_id];

    Generators gens { m_G.value(), H, m_Gi.value(), m_Hi.value() };
    return gens;
}
