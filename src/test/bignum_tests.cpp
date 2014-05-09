// Copyright (c) 2012-2014 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bignum.h"

#include <limits>
#include <stdint.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(bignum_tests)

// Unfortunately there's no standard way of preventing a function from being
// inlined, so we define a macro for it.
//
// You should use it like this:
//   NOINLINE void function() {...}
#if defined(__GNUC__)
// This also works and will be defined for any compiler implementing GCC
// extensions, such as Clang and ICC.
#define NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define NOINLINE __declspec(noinline)
#else
// We give out a warning because it impacts the correctness of one bignum test.
#warning You should define NOINLINE for your compiler.
#define NOINLINE
#endif

// For the following test case, it is useful to use additional tools.
//
// The simplest one to use is the compiler flag -ftrapv, which detects integer
// overflows and similar errors. However, due to optimizations and compilers
// taking advantage of undefined behavior sometimes it may not actually detect
// anything.
//
// You can also use compiler-based stack protection to possibly detect possible
// stack buffer overruns.
//
// For more accurate diagnostics, you can use an undefined arithmetic operation
// detector such as the clang's undefined behaviour checker.
// See also: http://clang.llvm.org/docs/UsersManual.html#controlling-code-generation
//
// It might also be useful to use Google's AddressSanitizer to detect
// stack buffer overruns, which valgrind can't currently detect.

// Let's force this code not to be inlined, in order to actually
// test a generic version of the function. This increases the chance
// that -ftrapv will detect overflows.
NOINLINE void mysetint64(CBigNum& num, int64_t n)
{
    num.setint64(n);
}

// For each number, we do 2 tests: one with inline code, then we reset the
// value to 0, then the second one with a non-inlined function.
BOOST_AUTO_TEST_CASE(bignum_setint64)
{
    int64_t n;

    {
        n = 0;
        CBigNum num(n);
        BOOST_CHECK(num.ToString() == "0");
        num.setulong(0);
        BOOST_CHECK(num.ToString() == "0");
        mysetint64(num, n);
        BOOST_CHECK(num.ToString() == "0");
    }
    {
        n = 1;
        CBigNum num(n);
        BOOST_CHECK(num.ToString() == "1");
        num.setulong(0);
        BOOST_CHECK(num.ToString() == "0");
        mysetint64(num, n);
        BOOST_CHECK(num.ToString() == "1");
    }
    {
        n = -1;
        CBigNum num(n);
        BOOST_CHECK(num.ToString() == "-1");
        num.setulong(0);
        BOOST_CHECK(num.ToString() == "0");
        mysetint64(num, n);
        BOOST_CHECK(num.ToString() == "-1");
    }
    {
        n = 5;
        CBigNum num(n);
        BOOST_CHECK(num.ToString() == "5");
        num.setulong(0);
        BOOST_CHECK(num.ToString() == "0");
        mysetint64(num, n);
        BOOST_CHECK(num.ToString() == "5");
    }
    {
        n = -5;
        CBigNum num(n);
        BOOST_CHECK(num.ToString() == "-5");
        num.setulong(0);
        BOOST_CHECK(num.ToString() == "0");
        mysetint64(num, n);
        BOOST_CHECK(num.ToString() == "-5");
    }
    {
        n = std::numeric_limits<int64_t>::min();
        CBigNum num(n);
        BOOST_CHECK(num.ToString() == "-9223372036854775808");
        num.setulong(0);
        BOOST_CHECK(num.ToString() == "0");
        mysetint64(num, n);
        BOOST_CHECK(num.ToString() == "-9223372036854775808");
    }
    {
        n = std::numeric_limits<int64_t>::max();
        CBigNum num(n);
        BOOST_CHECK(num.ToString() == "9223372036854775807");
        num.setulong(0);
        BOOST_CHECK(num.ToString() == "0");
        mysetint64(num, n);
        BOOST_CHECK(num.ToString() == "9223372036854775807");
    }
}


BOOST_AUTO_TEST_CASE(bignum_SetHex)
{
    std::string hexStr = "deecf97fd890808b9cc0f1b6a3e7a60b400f52710e6ad075b1340755bfa58cc9";
    CBigNum num;
    num.SetHex(hexStr);
    BOOST_CHECK_EQUAL(num.GetHex(), hexStr);
}

BOOST_AUTO_TEST_SUITE_END()
