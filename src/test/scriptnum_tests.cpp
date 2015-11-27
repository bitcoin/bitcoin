// Copyright (c) 2012-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "scriptnum10.h"
#include "script/script.h"
#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>
#include <limits.h>
#include <stdint.h>

BOOST_FIXTURE_TEST_SUITE(scriptnum_tests, BasicTestingSetup)

static const int64_t values[] = \
{ 0, 1, CHAR_MIN, CHAR_MAX, UCHAR_MAX, SHRT_MIN, USHRT_MAX, INT_MIN, INT_MAX, UINT_MAX, LONG_MIN, LONG_MAX };
static const int64_t offsets[] = { 1, 0x79, 0x80, 0x81, 0xFF, 0x7FFF, 0x8000, 0xFFFF, 0x10000};

static bool verify(const CScriptNum10& bignum, const CScriptNum& scriptnum)
{
    return bignum.getvch() == scriptnum.getvch() && bignum.getint() == scriptnum.getint();
}

static void CheckCreateVch(const int64_t& num)
{
    CScriptNum10 bignum(num);
    CScriptNum scriptnum(num);
    BOOST_CHECK(verify(bignum, scriptnum));

    std::vector<unsigned char> vch = bignum.getvch();

    CScriptNum10 bignum2(bignum.getvch(), false);
    vch = scriptnum.getvch();
    CScriptNum scriptnum2(scriptnum.getvch(), false);
    BOOST_CHECK(verify(bignum2, scriptnum2));

    CScriptNum10 bignum3(scriptnum2.getvch(), false);
    CScriptNum scriptnum3(bignum2.getvch(), false);
    BOOST_CHECK(verify(bignum3, scriptnum3));
}

static void CheckCreateInt(const int64_t& num)
{
    CScriptNum10 bignum(num);
    CScriptNum scriptnum(num);
    BOOST_CHECK(verify(bignum, scriptnum));
    BOOST_CHECK(verify(CScriptNum10(bignum.getint()), CScriptNum(scriptnum.getint())));
    BOOST_CHECK(verify(CScriptNum10(scriptnum.getint()), CScriptNum(bignum.getint())));
    BOOST_CHECK(verify(CScriptNum10(CScriptNum10(scriptnum.getint()).getint()), CScriptNum(CScriptNum(bignum.getint()).getint())));
}


static void CheckAdd(const int64_t& num1, const int64_t& num2)
{
    const CScriptNum10 bignum1(num1);
    const CScriptNum10 bignum2(num2);
    const CScriptNum scriptnum1(num1);
    const CScriptNum scriptnum2(num2);
    CScriptNum10 bignum3(num1);
    CScriptNum10 bignum4(num1);
    CScriptNum scriptnum3(num1);
    CScriptNum scriptnum4(num1);

    // int64_t overflow is undefined.
    bool invalid = (((num2 > 0) && (num1 > (std::numeric_limits<int64_t>::max() - num2))) ||
                    ((num2 < 0) && (num1 < (std::numeric_limits<int64_t>::min() - num2))));
    if (!invalid)
    {
        BOOST_CHECK(verify(bignum1 + bignum2, scriptnum1 + scriptnum2));
        BOOST_CHECK(verify(bignum1 + bignum2, scriptnum1 + num2));
        BOOST_CHECK(verify(bignum1 + bignum2, scriptnum2 + num1));
    }
}

static void CheckNegate(const int64_t& num)
{
    const CScriptNum10 bignum(num);
    const CScriptNum scriptnum(num);

    // -INT64_MIN is undefined
    if (num != std::numeric_limits<int64_t>::min())
        BOOST_CHECK(verify(-bignum, -scriptnum));
}

static void CheckSubtract(const int64_t& num1, const int64_t& num2)
{
    const CScriptNum10 bignum1(num1);
    const CScriptNum10 bignum2(num2);
    const CScriptNum scriptnum1(num1);
    const CScriptNum scriptnum2(num2);
    bool invalid = false;

    // int64_t overflow is undefined.
    invalid = ((num2 > 0 && num1 < std::numeric_limits<int64_t>::min() + num2) ||
               (num2 < 0 && num1 > std::numeric_limits<int64_t>::max() + num2));
    if (!invalid)
    {
        BOOST_CHECK(verify(bignum1 - bignum2, scriptnum1 - scriptnum2));
        BOOST_CHECK(verify(bignum1 - bignum2, scriptnum1 - num2));
    }

    invalid = ((num1 > 0 && num2 < std::numeric_limits<int64_t>::min() + num1) ||
               (num1 < 0 && num2 > std::numeric_limits<int64_t>::max() + num1));
    if (!invalid)
    {
        BOOST_CHECK(verify(bignum2 - bignum1, scriptnum2 - scriptnum1));
        BOOST_CHECK(verify(bignum2 - bignum1, scriptnum2 - num1));
    }
}

static void CheckCompare(const int64_t& num1, const int64_t& num2)
{
    const CScriptNum10 bignum1(num1);
    const CScriptNum10 bignum2(num2);
    const CScriptNum scriptnum1(num1);
    const CScriptNum scriptnum2(num2);

    BOOST_CHECK((bignum1 == bignum1) == (scriptnum1 == scriptnum1));
    BOOST_CHECK((bignum1 != bignum1) ==  (scriptnum1 != scriptnum1));
    BOOST_CHECK((bignum1 < bignum1) ==  (scriptnum1 < scriptnum1));
    BOOST_CHECK((bignum1 > bignum1) ==  (scriptnum1 > scriptnum1));
    BOOST_CHECK((bignum1 >= bignum1) ==  (scriptnum1 >= scriptnum1));
    BOOST_CHECK((bignum1 <= bignum1) ==  (scriptnum1 <= scriptnum1));

    BOOST_CHECK((bignum1 == bignum1) == (scriptnum1 == num1));
    BOOST_CHECK((bignum1 != bignum1) ==  (scriptnum1 != num1));
    BOOST_CHECK((bignum1 < bignum1) ==  (scriptnum1 < num1));
    BOOST_CHECK((bignum1 > bignum1) ==  (scriptnum1 > num1));
    BOOST_CHECK((bignum1 >= bignum1) ==  (scriptnum1 >= num1));
    BOOST_CHECK((bignum1 <= bignum1) ==  (scriptnum1 <= num1));

    BOOST_CHECK((bignum1 == bignum2) ==  (scriptnum1 == scriptnum2));
    BOOST_CHECK((bignum1 != bignum2) ==  (scriptnum1 != scriptnum2));
    BOOST_CHECK((bignum1 < bignum2) ==  (scriptnum1 < scriptnum2));
    BOOST_CHECK((bignum1 > bignum2) ==  (scriptnum1 > scriptnum2));
    BOOST_CHECK((bignum1 >= bignum2) ==  (scriptnum1 >= scriptnum2));
    BOOST_CHECK((bignum1 <= bignum2) ==  (scriptnum1 <= scriptnum2));

    BOOST_CHECK((bignum1 == bignum2) ==  (scriptnum1 == num2));
    BOOST_CHECK((bignum1 != bignum2) ==  (scriptnum1 != num2));
    BOOST_CHECK((bignum1 < bignum2) ==  (scriptnum1 < num2));
    BOOST_CHECK((bignum1 > bignum2) ==  (scriptnum1 > num2));
    BOOST_CHECK((bignum1 >= bignum2) ==  (scriptnum1 >= num2));
    BOOST_CHECK((bignum1 <= bignum2) ==  (scriptnum1 <= num2));
}

static void RunCreate(const int64_t& num)
{
    CheckCreateInt(num);
    CScriptNum scriptnum(num);
    if (scriptnum.getvch().size() <= CScriptNum::nDefaultMaxNumSize)
        CheckCreateVch(num);
    else
    {
        BOOST_CHECK_THROW (CheckCreateVch(num), scriptnum10_error);
    }
}

static void RunOperators(const int64_t& num1, const int64_t& num2)
{
    CheckAdd(num1, num2);
    CheckSubtract(num1, num2);
    CheckNegate(num1);
    CheckCompare(num1, num2);
}

BOOST_AUTO_TEST_CASE(creation)
{
    for(size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i)
    {
        for(size_t j = 0; j < sizeof(offsets) / sizeof(offsets[0]); ++j)
        {
            RunCreate(values[i]);
            RunCreate(values[i] + offsets[j]);
            RunCreate(values[i] - offsets[j]);
        }
    }
}

BOOST_AUTO_TEST_CASE(operators)
{
    for(size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i)
    {
        for(size_t j = 0; j < sizeof(offsets) / sizeof(offsets[0]); ++j)
        {
            RunOperators(values[i], values[i]);
            RunOperators(values[i], -values[i]);
            RunOperators(values[i], values[j]);
            RunOperators(values[i], -values[j]);
            RunOperators(values[i] + values[j], values[j]);
            RunOperators(values[i] + values[j], -values[j]);
            RunOperators(values[i] - values[j], values[j]);
            RunOperators(values[i] - values[j], -values[j]);
            RunOperators(values[i] + values[j], values[i] + values[j]);
            RunOperators(values[i] + values[j], values[i] - values[j]);
            RunOperators(values[i] - values[j], values[i] + values[j]);
            RunOperators(values[i] - values[j], values[i] - values[j]);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
