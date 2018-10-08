// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <test/gen/bloom_gen.h>
#include <test/gen/crypto_gen.h>

#include <test/setup_common.h>

#include <boost/test/unit_test.hpp>
#include <rapidcheck/Gen.h>
#include <rapidcheck/boost_test.h>
#include <rapidcheck/gen/Arbitrary.h>

#include <streams.h>

BOOST_FIXTURE_TEST_SUITE(bloom_properties, BasicTestingSetup)

RC_BOOST_PROP(no_false_negatives, (CBloomFilter bloom_filter, const uint256& hash))
{
    bloom_filter.insert(hash);
    bool result = bloom_filter.contains(hash);
    RC_ASSERT(result);
}

RC_BOOST_PROP(serialization_symmetry, (const CBloomFilter& bloom_filter))
{
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << bloom_filter;
    CBloomFilter bloom_filter2;
    ss >> bloom_filter2;
    CDataStream ss1(SER_NETWORK, PROTOCOL_VERSION);
    ss << bloom_filter;
    ss1 << bloom_filter2;
    RC_ASSERT(ss.str() == ss1.str());
}
BOOST_AUTO_TEST_SUITE_END()
