// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_RANGE_PROOF_GENERATORS_H
#define NAVCOIN_BLSCT_ARITH_RANGE_PROOF_GENERATORS_H

#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>

#include <blsct/arith/elements.h>
#include <blsct/arith/g1point.h>
#include <blsct/arith/range_proof/config.h>
#include <blsct/arith/scalar.h>
#include <ctokens/tokenid.h>

class Generators
{
public:
    Generators();

    G1Point H(const TokenId& token_id);

    G1Point G() const;
    G1Points Gi() const;
    G1Points Hi() const;

private:
    static void Init();

    static G1Point GetGenerator(
        const G1Point& p,
        const size_t index,
        const TokenId& token_id
    );

    // H generator is created for each instance and cached
    inline static std::map<const TokenId, const G1Point> m_H_cache;

    // G, Gi, Hi generators are shared among all instances
    static G1Point m_G;
    static G1Points m_Gi;
    static G1Points m_Hi;

    inline static boost::mutex m_init_mutex;
    inline static bool m_is_static_values_initialized = false;
};

#endif // NAVCOIN_BLSCT_ARITH_RANGE_PROOF_GENERATORS_H
