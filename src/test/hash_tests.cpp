

#include "hash.h"
#include "util.h"

#include <vector>

#include <boost/test/unit_test.hpp>

using namespace std;

BOOST_AUTO_TEST_SUITE(hash_tests)

BOOST_AUTO_TEST_CASE(murmurhash3)
{

#define T(expected, seed, data) BOOST_CHECK_EQUAL(MurmurHash3(seed, ParseHex(data)), expected)

    // Test MurmurHash3 with various inputs. Of course this is retested in the
    // bloom filter tests - they would fail if MurmurHash3() had any problems -
    // but is useful for those trying to implement Bitcoin libraries as a
    // source of test data for their MurmurHash3() primitive during
    // development.
    //
    // The magic number 0xFBA4C795 comes from CBloomFilter::Hash()

    T(0x00000000, 0x00000000, "");
    T(0x6a396f08, 0xFBA4C795, "");
    T(0x81f16f39, 0xffffffff, "");

    T(0x514e28b7, 0x00000000, "00");
    T(0xea3f0b17, 0xFBA4C795, "00");
    T(0xfd6cf10d, 0x00000000, "ff");

    T(0x16c6b7ab, 0x00000000, "0011");
    T(0x8eb51c3d, 0x00000000, "001122");
    T(0xb4471bf8, 0x00000000, "00112233");
    T(0xe2301fa8, 0x00000000, "0011223344");
    T(0xfc2e4a15, 0x00000000, "001122334455");
    T(0xb074502c, 0x00000000, "00112233445566");
    T(0x8034d2a0, 0x00000000, "0011223344556677");
    T(0xb4698def, 0x00000000, "001122334455667788");

#undef T
}

BOOST_AUTO_TEST_SUITE_END()
