#include "omnicore/parsing.h"

#include "test/test_bitcoin.h"

#include <boost/test/unit_test.hpp>

#include <stdint.h>

BOOST_FIXTURE_TEST_SUITE(omnicore_swapbyteorder_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(swapbyteorder_cycle)
{
    uint16_t a1 = 41959U, a2 = a1;

    SwapByteOrder16(a2);
    SwapByteOrder16(a2);
    BOOST_CHECK_EQUAL(a1, a2); // Should be equal, swapping twice brings us back to the original value

    uint32_t b1 = 16909060U, b2 = b1;
    SwapByteOrder32(b2);
    SwapByteOrder32(b2);    
    BOOST_CHECK_EQUAL(b1, b2); // Should be equal, swapping twice brings us back to the original value

    uint64_t c1 = 722385979038285U, c2 = c1;
    SwapByteOrder64(c2);
    SwapByteOrder64(c2);
    BOOST_CHECK_EQUAL(c1, c2); // Should be equal, swapping twice brings us back to the original value
}


BOOST_AUTO_TEST_SUITE_END()
