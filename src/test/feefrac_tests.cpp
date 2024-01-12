// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/feefrac.h>
#include <random.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(feefrac_tests)

BOOST_AUTO_TEST_CASE(feefrac_operators)
{
    FeeFrac p1{1000, 100}, p2{500, 300};
    FeeFrac sum{1500, 400};
    FeeFrac diff{500, -200};
    FeeFrac empty{0, 0};
    FeeFrac zero_fee{0, 1}; // zero-fee allowed

    BOOST_CHECK(empty == FeeFrac{}); // same as no-args

    BOOST_CHECK(p1 == p1);
    BOOST_CHECK(p1 + p2 == sum);
    BOOST_CHECK(p1 - p2 == diff);

    FeeFrac p3{2000, 200};
    BOOST_CHECK(p1 != p3); // feefracs only equal if both fee and size are same
    BOOST_CHECK(p2 != p3);

    FeeFrac p4{3000, 300};
    BOOST_CHECK(p1 == p4-p3);
    BOOST_CHECK(p1 + p3 == p4);

    // Fee-rate comparison
    BOOST_CHECK(p1 > p2);
    BOOST_CHECK(p1 >= p2);
    BOOST_CHECK(p1 >= p4-p3);
    BOOST_CHECK(!(p1 >> p3)); // not strictly better
    BOOST_CHECK(p1 >> p2); // strictly greater feerate

    BOOST_CHECK(p2 < p1);
    BOOST_CHECK(p2 <= p1);
    BOOST_CHECK(p1 <= p4-p3);
    BOOST_CHECK(!(p3 << p1)); // not strictly worse
    BOOST_CHECK(p2 << p1); // strictly lower feerate

    // "empty" comparisons
    BOOST_CHECK(!(p1 >> empty)); // << will always result in false
    BOOST_CHECK(!(p1 << empty));
    BOOST_CHECK(!(empty >> empty));
    BOOST_CHECK(!(empty << empty));

    // empty is always bigger than everything else
    BOOST_CHECK(empty > p1);
    BOOST_CHECK(empty > p2);
    BOOST_CHECK(empty > p3);
    BOOST_CHECK(empty >= p1);
    BOOST_CHECK(empty >= p2);
    BOOST_CHECK(empty >= p3);

    // check "max" values for comparison
    FeeFrac oversized_1{4611686000000, 4000000};
    FeeFrac oversized_2{184467440000000, 100000};

    BOOST_CHECK(oversized_1 < oversized_2);
    BOOST_CHECK(oversized_1 <= oversized_2);
    BOOST_CHECK(oversized_1 << oversized_2);
    BOOST_CHECK(oversized_1 != oversized_2);

    // Tests paths that use double arithmetic
    FeeFrac busted{(static_cast<int64_t>(INT32_MAX)) + 1, INT32_MAX};
    BOOST_CHECK(!(busted < busted));

    FeeFrac max_fee{2100000000000000, INT32_MAX};
    BOOST_CHECK(!(max_fee < max_fee));
    BOOST_CHECK(!(max_fee > max_fee));
    BOOST_CHECK(max_fee <= max_fee);
    BOOST_CHECK(max_fee >= max_fee);

    FeeFrac max_fee2{1, 1};
    BOOST_CHECK(max_fee >= max_fee2);

}

BOOST_AUTO_TEST_CASE(build_diagram_test)
{
    FeeFrac p1{1000, 100};
    FeeFrac empty{0, 0};
    FeeFrac zero_fee{0, 1};
    FeeFrac oversized_1{4611686000000, 4000000};
    FeeFrac oversized_2{184467440000000, 100000};

    // Diagram-building will reorder chunks
    std::vector<FeeFrac> chunks;
    chunks.push_back(p1);
    chunks.push_back(zero_fee);
    chunks.push_back(empty);
    chunks.push_back(oversized_1);
    chunks.push_back(oversized_2);

    // Caller in charge of sorting chunks in case chunk size limit
    // differs from cluster size limit
    std::sort(chunks.begin(), chunks.end(), [](const FeeFrac& a, const FeeFrac& b) { return a > b; });

    // Chunks are now sorted in reverse order (largest first)
    BOOST_CHECK(chunks[0] == empty); // empty is considered "highest" fee
    BOOST_CHECK(chunks[1] == oversized_2);
    BOOST_CHECK(chunks[2] == oversized_1);
    BOOST_CHECK(chunks[3] == p1);
    BOOST_CHECK(chunks[4] == zero_fee);

    std::vector<FeeFrac> generated_diagram{BuildDiagramFromChunks(chunks)};
    BOOST_CHECK(generated_diagram.size() == 1 + chunks.size());

    // Prepended with an empty, then the chunks summed in order as above
    BOOST_CHECK(generated_diagram[0] == empty);
    BOOST_CHECK(generated_diagram[1] == empty);
    BOOST_CHECK(generated_diagram[2] == oversized_2);
    BOOST_CHECK(generated_diagram[3] == oversized_2 + oversized_1);
    BOOST_CHECK(generated_diagram[4] == oversized_2 + oversized_1 + p1);
    BOOST_CHECK(generated_diagram[5] == oversized_2 + oversized_1 + p1 + zero_fee);
}

BOOST_AUTO_TEST_SUITE_END()
