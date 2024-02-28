// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_RANGE_PROOF_GENERATORS_H
#define NAVCOIN_BLSCT_RANGE_PROOF_GENERATORS_H

#include <blsct/arith/elements.h>
#include <blsct/building_block/generator_deriver.h>
#include <blsct/range_proof/setup.h>
#include <ctokens/tokenid.h>

#include <mutex>

namespace range_proof {

template <typename T>
struct Generators {
    using Point = typename T::Point;
    using Points = Elements<Point>;

public:
    Generators(
        const Point& H,
        const Point& G,
        const Points& Gi,
        const Points& Hi
    ) : H{H}, G{G}, Gi{Gi}, Hi{Hi} {}

    Points GetGiSubset(const size_t& size) const;
    Points GetHiSubset(const size_t& size) const;

    const Point H;
    const Point G;
    const Points Gi;
    const Points Hi;
};

/**
 * - G generator is derived from token_id
 * - H generator points to the base point
 * - Gi and Hi generators are derived from the base point
 *   and the default token_id at initialization time
 *
 * G is used for amounts and H is used for randomness
 * following the Bulletproofs paper
 *
 * When a tx is valid, the sum of the input and output
 * becomes zero, and that clears the G term and only
 * the H term remains.
 *
 * Since H is the base point to H, the remaining term
 * becomes the publickey and is later used later for
 * signature verification.
 */
template <typename T>
class GeneratorsFactory
{
    using Point = typename T::Point;
    using Points = Elements<Point>;
    using Seed = typename GeneratorDeriver<T>::Seed;

public:
    GeneratorsFactory();

    // derives G from the seed. other generators
    // are shared amongst all instances
    Generators<T> GetInstance(const Seed& seed) const;

private:
    inline const static GeneratorDeriver m_deriver =
        GeneratorDeriver<typename T::Point>("bulletproofs");

    // G generators are cached
    inline static std::map<const Seed, const Point> m_G_cache;

    inline static Point m_H;
    inline static Points m_Gi;
    inline static Points m_Hi;

    inline static std::mutex m_init_mutex;
    inline static bool m_is_initialized = false;
};

}

#endif // NAVCOIN_BLSCT_RANGE_PROOF_GENERATORS_H
