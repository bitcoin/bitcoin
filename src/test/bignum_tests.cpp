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
// detector such as the clang-based tool:
//
// "IOC: An Integer Overflow Checker for C/C++"
//
// Available at: http://embed.cs.utah.edu/ioc/
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


BOOST_AUTO_TEST_CASE(bignum_SetCompact)
{
    CBigNum num;
    num.SetCompact(0);
    BOOST_CHECK_EQUAL(num.GetHex(), "0");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0U);

    num.SetCompact(0x00123456);
    BOOST_CHECK_EQUAL(num.GetHex(), "0");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0U);

    num.SetCompact(0x01003456);
    BOOST_CHECK_EQUAL(num.GetHex(), "0");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0U);

    num.SetCompact(0x02000056);
    BOOST_CHECK_EQUAL(num.GetHex(), "0");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0U);

    num.SetCompact(0x03000000);
    BOOST_CHECK_EQUAL(num.GetHex(), "0");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0U);

    num.SetCompact(0x04000000);
    BOOST_CHECK_EQUAL(num.GetHex(), "0");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0U);

    num.SetCompact(0x00923456);
    BOOST_CHECK_EQUAL(num.GetHex(), "0");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0U);

    num.SetCompact(0x01803456);
    BOOST_CHECK_EQUAL(num.GetHex(), "0");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0U);

    num.SetCompact(0x02800056);
    BOOST_CHECK_EQUAL(num.GetHex(), "0");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0U);

    num.SetCompact(0x03800000);
    BOOST_CHECK_EQUAL(num.GetHex(), "0");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0U);
    
    num.SetCompact(0x04800000);
    BOOST_CHECK_EQUAL(num.GetHex(), "0");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0U);

    num.SetCompact(0x01123456);
    BOOST_CHECK_EQUAL(num.GetHex(), "12");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0x01120000U);

    // Make sure that we don't generate compacts with the 0x00800000 bit set
    num = 0x80;
    BOOST_CHECK_EQUAL(num.GetCompact(), 0x02008000U);

    num.SetCompact(0x01fedcba);
    BOOST_CHECK_EQUAL(num.GetHex(), "-7e");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0x01fe0000U);

    num.SetCompact(0x02123456);
    BOOST_CHECK_EQUAL(num.GetHex(), "1234");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0x02123400U);

    num.SetCompact(0x03123456);
    BOOST_CHECK_EQUAL(num.GetHex(), "123456");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0x03123456U);

    num.SetCompact(0x04123456);
    BOOST_CHECK_EQUAL(num.GetHex(), "12345600");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0x04123456U);

    num.SetCompact(0x04923456);
    BOOST_CHECK_EQUAL(num.GetHex(), "-12345600");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0x04923456U);

    num.SetCompact(0x05009234);
    BOOST_CHECK_EQUAL(num.GetHex(), "92340000");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0x05009234U);

    num.SetCompact(0x20123456);
    BOOST_CHECK_EQUAL(num.GetHex(), "1234560000000000000000000000000000000000000000000000000000000000");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0x20123456U);

    num.SetCompact(0xff123456);
    BOOST_CHECK_EQUAL(num.GetHex(), "123456000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
    BOOST_CHECK_EQUAL(num.GetCompact(), 0xff123456U);
}

BOOST_AUTO_TEST_CASE(bignum_SetHex)
{
    std::string hexStr = "deecf97fd890808b9cc0f1b6a3e7a60b400f52710e6ad075b1340755bfa58cc9";
    CBigNum num;
    num.SetHex(hexStr);
    BOOST_CHECK_EQUAL(num.GetHex(), hexStr);
}

BOOST_AUTO_TEST_SUITE_END()
