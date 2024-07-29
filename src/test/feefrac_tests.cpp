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

    BOOST_CHECK_EQUAL(zero_fee.EvaluateFee(0), 0);
    BOOST_CHECK_EQUAL(zero_fee.EvaluateFee(1), 0);
    BOOST_CHECK_EQUAL(zero_fee.EvaluateFee(1000000), 0);
    BOOST_CHECK_EQUAL(zero_fee.EvaluateFee(0x7fffffff), 0);

    BOOST_CHECK_EQUAL(p1.EvaluateFee(0), 0);
    BOOST_CHECK_EQUAL(p1.EvaluateFee(1), 10);
    BOOST_CHECK_EQUAL(p1.EvaluateFee(100000000), 1000000000);
    BOOST_CHECK_EQUAL(p1.EvaluateFee(0x7fffffff), int64_t(0x7fffffff) * 10);

    FeeFrac neg{-1001, 100};
    BOOST_CHECK_EQUAL(neg.EvaluateFee(0), 0);
    BOOST_CHECK_EQUAL(neg.EvaluateFee(1), -11);
    BOOST_CHECK_EQUAL(neg.EvaluateFee(2), -21);
    BOOST_CHECK_EQUAL(neg.EvaluateFee(3), -31);
    BOOST_CHECK_EQUAL(neg.EvaluateFee(100), -1001);
    BOOST_CHECK_EQUAL(neg.EvaluateFee(101), -1012);
    BOOST_CHECK_EQUAL(neg.EvaluateFee(100000000), -1001000000);
    BOOST_CHECK_EQUAL(neg.EvaluateFee(100000001), -1001000011);
    BOOST_CHECK_EQUAL(neg.EvaluateFee(0x7fffffff), -21496311307);

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

    BOOST_CHECK_EQUAL(oversized_1.EvaluateFee(0), 0);
    BOOST_CHECK_EQUAL(oversized_1.EvaluateFee(1), 1152921);
    BOOST_CHECK_EQUAL(oversized_1.EvaluateFee(2), 2305843);
    BOOST_CHECK_EQUAL(oversized_1.EvaluateFee(1548031267), 1784758530396540);

    // Test cases on the threshold where FeeFrac::EvaluateFee start using Mul/Div.
    BOOST_CHECK_EQUAL(FeeFrac(0x1ffffffff, 123456789).EvaluateFee(98765432), 6871947728);
    BOOST_CHECK_EQUAL(FeeFrac(0x200000000, 123456789).EvaluateFee(98765432), 6871947729);
    BOOST_CHECK_EQUAL(FeeFrac(0x200000001, 123456789).EvaluateFee(98765432), 6871947730);

    // Tests paths that use double arithmetic
    FeeFrac busted{(static_cast<int64_t>(INT32_MAX)) + 1, INT32_MAX};
    BOOST_CHECK(!(busted < busted));

    FeeFrac max_fee{2100000000000000, INT32_MAX};
    BOOST_CHECK(!(max_fee < max_fee));
    BOOST_CHECK(!(max_fee > max_fee));
    BOOST_CHECK(max_fee <= max_fee);
    BOOST_CHECK(max_fee >= max_fee);

    BOOST_CHECK_EQUAL(max_fee.EvaluateFee(0), 0);
    BOOST_CHECK_EQUAL(max_fee.EvaluateFee(1), 977888);
    BOOST_CHECK_EQUAL(max_fee.EvaluateFee(2), 1955777);
    BOOST_CHECK_EQUAL(max_fee.EvaluateFee(3), 2933666);
    BOOST_CHECK_EQUAL(max_fee.EvaluateFee(1256796054), 1229006664189047);
    BOOST_CHECK_EQUAL(max_fee.EvaluateFee(INT32_MAX), 2100000000000000);

    FeeFrac max_fee2{1, 1};
    BOOST_CHECK(max_fee >= max_fee2);

}

BOOST_AUTO_TEST_SUITE_END()
