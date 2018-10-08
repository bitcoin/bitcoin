// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_TEST_GEN_BLOOM_GEN_H
#define BITCOIN_TEST_GEN_BLOOM_GEN_H

#include <bloom.h>
#include <merkleblock.h>

#include <math.h>

#include <rapidcheck/Gen.h>
#include <rapidcheck/gen/Arbitrary.h>

/** Generates a double between [0,1) */
rc::Gen<double> BetweenZeroAndOne();

rc::Gen<std::tuple<unsigned int, double, unsigned int, unsigned int>> BloomFilterPrimitives();

namespace rc {
/** Generator for a new CBloomFilter*/
template <>
struct Arbitrary<CBloomFilter> {
    static Gen<CBloomFilter> arbitrary()
    {
        return gen::map(BloomFilterPrimitives(), [](const std::tuple<unsigned int, double, unsigned int, unsigned int>& primitives) {
            unsigned int num_elements;
            double fp_rate;
            unsigned int n_tweak_in;
            unsigned int bloom_flag;
            std::tie(num_elements, fp_rate, n_tweak_in, bloom_flag) = primitives;
            return CBloomFilter(num_elements, fp_rate, n_tweak_in, bloom_flag);
        });
    };
};
} //namespace rc

/** Returns a bloom filter loaded with the returned uint256s */
rc::Gen<std::pair<CBloomFilter, std::vector<uint256>>> LoadedBloomFilter();

/** Loads an arbitrary bloom filter with the given hashes */
rc::Gen<std::pair<CBloomFilter, std::vector<uint256>>> LoadBloomFilter(const std::vector<uint256>& hashes);

#endif
