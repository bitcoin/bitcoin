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

struct Generators
{
    std::reference_wrapper<G1Point> G;
    std::reference_wrapper<G1Point> H;
    std::reference_wrapper<G1Points> Gi;
    std::reference_wrapper<G1Points> Hi;
};

/**
 * shared generators are stored to an GeneratorFactory instance member
 * variables instead of the class static member variables. it is because
 * initializing G1Point(s) requires mcl library to be initilized before,
 * but that is not feasible when G1Point(s) are static.
 */
class GeneratorsFactory
{
public:
    GeneratorsFactory();
    Generators GetInstance(const TokenId& token_id);

private:
    G1Point GetGenerator(
        const G1Point& p,
        const size_t index,
        const TokenId& token_id
    );

    // H generator is created for each instance and cached
    std::map<const TokenId, const G1Point> m_H_cache;

    G1Point m_G;
    G1Points m_Gi;
    G1Points m_Hi;

    inline static boost::mutex m_init_mutex;
    inline static bool m_is_initialized = false;
};

#endif // NAVCOIN_BLSCT_ARITH_RANGE_PROOF_GENERATORS_H
