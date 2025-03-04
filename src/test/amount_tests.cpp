// Copyright (c) 2016-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <policy/feerate.h>

#include <limits>

#include <gtest/gtest.h>

TEST(AmountTests, MoneyRangeTest)
{
    EXPECT_FALSE(MoneyRange(CAmount(-1)));
    EXPECT_TRUE(MoneyRange(CAmount(0)));
    EXPECT_TRUE(MoneyRange(CAmount(1)));
    EXPECT_TRUE(MoneyRange(MAX_MONEY));
    EXPECT_FALSE(MoneyRange(MAX_MONEY + CAmount(1)));
}

TEST(AmountTests, GetFeeTest)
{
    CFeeRate feeRate, altFeeRate;

    feeRate = CFeeRate(0);
    // Must always return 0
    ASSERT_EQ(feeRate.GetFee(0), CAmount(0));
    ASSERT_EQ(feeRate.GetFee(1e5), CAmount(0));

    feeRate = CFeeRate(1000);
    // Must always just return the arg
    ASSERT_EQ(feeRate.GetFee(0), CAmount(0));
    ASSERT_EQ(feeRate.GetFee(1), CAmount(1));
    ASSERT_EQ(feeRate.GetFee(121), CAmount(121));
    ASSERT_EQ(feeRate.GetFee(999), CAmount(999));
    ASSERT_EQ(feeRate.GetFee(1e3), CAmount(1e3));
    ASSERT_EQ(feeRate.GetFee(9e3), CAmount(9e3));

    feeRate = CFeeRate(-1000);
    // Must always just return -1 * arg
    ASSERT_EQ(feeRate.GetFee(0), CAmount(0));
    ASSERT_EQ(feeRate.GetFee(1), CAmount(-1));
    ASSERT_EQ(feeRate.GetFee(121), CAmount(-121));
    ASSERT_EQ(feeRate.GetFee(999), CAmount(-999));
    ASSERT_EQ(feeRate.GetFee(1e3), CAmount(-1e3));
    ASSERT_EQ(feeRate.GetFee(9e3), CAmount(-9e3));

    feeRate = CFeeRate(123);
    // Rounds up the result, if not integer
    ASSERT_EQ(feeRate.GetFee(0), CAmount(0));
    ASSERT_EQ(feeRate.GetFee(8), CAmount(1)); // Special case: returns 1 instead of 0
    ASSERT_EQ(feeRate.GetFee(9), CAmount(2));
    ASSERT_EQ(feeRate.GetFee(121), CAmount(15));
    ASSERT_EQ(feeRate.GetFee(122), CAmount(16));
    ASSERT_EQ(feeRate.GetFee(999), CAmount(123));
    ASSERT_EQ(feeRate.GetFee(1e3), CAmount(123));
    ASSERT_EQ(feeRate.GetFee(9e3), CAmount(1107));

    feeRate = CFeeRate(-123);
    // Truncates the result, if not integer
    ASSERT_EQ(feeRate.GetFee(0), CAmount(0));
    ASSERT_EQ(feeRate.GetFee(8), CAmount(-1)); // Special case: returns -1 instead of 0
    ASSERT_EQ(feeRate.GetFee(9), CAmount(-1));

    // check alternate constructor
    feeRate = CFeeRate(1000);
    altFeeRate = CFeeRate(feeRate);
    ASSERT_EQ(feeRate.GetFee(100), altFeeRate.GetFee(100));

    // Check full constructor
    ASSERT_TRUE(CFeeRate(CAmount(-1), 0) == CFeeRate(0));
    ASSERT_TRUE(CFeeRate(CAmount(0), 0) == CFeeRate(0));
    ASSERT_TRUE(CFeeRate(CAmount(1), 0) == CFeeRate(0));
    // default value
    ASSERT_TRUE(CFeeRate(CAmount(-1), 1000) == CFeeRate(-1));
    ASSERT_TRUE(CFeeRate(CAmount(0), 1000) == CFeeRate(0));
    ASSERT_TRUE(CFeeRate(CAmount(1), 1000) == CFeeRate(1));
    // lost precision (can only resolve satoshis per kB)
    ASSERT_TRUE(CFeeRate(CAmount(1), 1001) == CFeeRate(0));
    ASSERT_TRUE(CFeeRate(CAmount(2), 1001) == CFeeRate(1));
    // some more integer checks
    ASSERT_TRUE(CFeeRate(CAmount(26), 789) == CFeeRate(32));
    ASSERT_TRUE(CFeeRate(CAmount(27), 789) == CFeeRate(34));
    // Maximum size in bytes, should not crash
    CFeeRate(MAX_MONEY, std::numeric_limits<uint32_t>::max()).GetFeePerK();

    // check multiplication operator
    // check multiplying by zero
    feeRate = CFeeRate(1000);
    ASSERT_TRUE(0 * feeRate == CFeeRate(0));
    ASSERT_TRUE(feeRate * 0 == CFeeRate(0));
    // check multiplying by a positive integer
    ASSERT_TRUE(3 * feeRate == CFeeRate(3000));
    ASSERT_TRUE(feeRate * 3 == CFeeRate(3000));
    // check multiplying by a negative integer
    ASSERT_TRUE(-3 * feeRate == CFeeRate(-3000));
    ASSERT_TRUE(feeRate * -3 == CFeeRate(-3000));
    // check commutativity
    ASSERT_TRUE(2 * feeRate == feeRate * 2);
    // check with large numbers
    int largeNumber = 1000000;
    ASSERT_TRUE(largeNumber * feeRate == feeRate * largeNumber);
    // check boundary values
    int maxInt = std::numeric_limits<int>::max();
    feeRate = CFeeRate(maxInt);
    ASSERT_TRUE(feeRate * 2 == CFeeRate(static_cast<int64_t>(maxInt) * 2));
    ASSERT_TRUE(2 * feeRate == CFeeRate(static_cast<int64_t>(maxInt) * 2));
    // check with zero fee rate
    feeRate = CFeeRate(0);
    ASSERT_TRUE(feeRate * 5 == CFeeRate(0));
    ASSERT_TRUE(5 * feeRate == CFeeRate(0));
}

TEST(AmountTests, BinaryOperatorTest)
{
    CFeeRate a, b;
    a = CFeeRate(1);
    b = CFeeRate(2);
    ASSERT_TRUE(a < b);
    ASSERT_TRUE(b > a);
    ASSERT_TRUE(a == a);
    ASSERT_TRUE(a <= b);
    ASSERT_TRUE(a <= a);
    ASSERT_TRUE(b >= a);
    ASSERT_TRUE(b >= b);
    // a should be 0.00000002 BTC/kvB now
    a += a;
    ASSERT_TRUE(a == b);
}

TEST(AmountTests, ToStringTest)
{
    CFeeRate feeRate;
    feeRate = CFeeRate(1);
    ASSERT_EQ(feeRate.ToString(), "0.00000001 BTC/kvB");
    ASSERT_EQ(feeRate.ToString(FeeEstimateMode::BTC_KVB), "0.00000001 BTC/kvB");
    ASSERT_EQ(feeRate.ToString(FeeEstimateMode::SAT_VB), "0.001 sat/vB");
}
