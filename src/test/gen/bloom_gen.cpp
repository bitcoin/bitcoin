// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <test/gen/bloom_gen.h>
#include <test/gen/crypto_gen.h>

#include <bloom.h>
#include <math.h>

#include <rapidcheck/Gen.h>
#include <rapidcheck/gen/Arbitrary.h>
#include <rapidcheck/gen/Container.h>
#include <rapidcheck/gen/Numeric.h>
#include <rapidcheck/gen/Predicate.h>
#include <rapidcheck/gen/Tuple.h>

/** Generates a double between [0,1) */
rc::Gen<double> BetweenZeroAndOne()
{
    return rc::gen::map(rc::gen::arbitrary<double>(), [](double x) {
        double result = abs(fmod(x, 1));
        assert(result >= 0 && result < 1);
        return result;
    });
}

rc::Gen<unsigned int> Between1And100()
{
    return rc::gen::inRange<unsigned int>(1, 100);
}
/** Generates the C++ primitives used to create a bloom filter */
rc::Gen<std::tuple<unsigned int, double, unsigned int, unsigned int>> BloomFilterPrimitives()
{
    return rc::gen::tuple(Between1And100(),
        BetweenZeroAndOne(), rc::gen::arbitrary<unsigned int>(),
        rc::gen::inRange<unsigned int>(0, 3));
}

/** Returns a bloom filter loaded with the given uint256s */
rc::Gen<std::pair<CBloomFilter, std::vector<uint256>>> LoadedBloomFilter()
{
    return rc::gen::map(rc::gen::pair(rc::gen::arbitrary<CBloomFilter>(), rc::gen::arbitrary<std::vector<uint256>>()),
        [](const std::pair<CBloomFilter, const std::vector<uint256>&>& primitives) {
            CBloomFilter bloomFilter = primitives.first;
            std::vector<uint256> hashes = primitives.second;
            for (unsigned int i = 0; i < hashes.size(); i++) {
                bloomFilter.insert(hashes[i]);
            }
            return std::make_pair(bloomFilter, hashes);
        });
}

/** Loads an arbitrary bloom filter with the given hashes */
rc::Gen<std::pair<CBloomFilter, std::vector<uint256>>> LoadBloomFilter(const std::vector<uint256>& hashes)
{
    return rc::gen::map(rc::gen::arbitrary<CBloomFilter>(), [&hashes](CBloomFilter bloomFilter) {
        for (unsigned int i = 0; i < hashes.size(); i++) {
            bloomFilter.insert(hashes[i]);
        }
        return std::make_pair(bloomFilter, hashes);
    });
}
