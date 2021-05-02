// Copyright (c) 2019-2020 The XBit Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <test/util/setup_common.h>

BOOST_FIXTURE_TEST_SUITE(compilerbug_tests, BasicTestingSetup)

#if defined(__GNUC__)
// This block will also be built under clang, which is fine (as it supports noinline)
void __attribute__ ((noinline)) set_one(unsigned char* ptr)
{
    *ptr = 1;
}

int __attribute__ ((noinline)) check_zero(unsigned char const* in, unsigned int len)
{
    for (unsigned int i = 0; i < len; ++i) {
        if (in[i] != 0) return 0;
    }
    return 1;
}

void set_one_on_stack() {
    unsigned char buf[1];
    set_one(buf);
}

BOOST_AUTO_TEST_CASE(gccbug_90348) {
    // Test for GCC bug 90348. See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90348
    for (int i = 0; i <= 4; ++i) {
        unsigned char in[4];
        for (int j = 0; j < i; ++j) {
            in[j] = 0;
            set_one_on_stack(); // Apparently modifies in[0]
        }
        BOOST_CHECK(check_zero(in, i));
    }
}
#endif

BOOST_AUTO_TEST_SUITE_END()
