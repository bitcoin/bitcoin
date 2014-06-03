#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>

#include "serialize.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(serialize_tests)

BOOST_AUTO_TEST_CASE(varints)
{
    // encode

    CDataStream ss(SER_DISK, 0);
    CDataStream::size_type size = 0;
    for (int i = 0; i < 100000; i++) {
        ss << VARINT(i);
        size += ::GetSerializeSize(VARINT(i), 0, 0);
        BOOST_CHECK(size == ss.size());
    }

    for (uint64 i = 0;  i < 100000000000ULL; i += 999999937) {
        ss << VARINT(i);
        size += ::GetSerializeSize(VARINT(i), 0, 0);
        BOOST_CHECK(size == ss.size());
    }

    // decode
    for (int i = 0; i < 100000; i++) {
        int j = -1;
        ss >> VARINT(j);
        BOOST_CHECK_MESSAGE(i == j, "decoded:" << j << " expected:" << i);
    }

    for (uint64 i = 0;  i < 100000000000ULL; i += 999999937) {
        uint64 j = -1;
        ss >> VARINT(j);
        BOOST_CHECK_MESSAGE(i == j, "decoded:" << j << " expected:" << i);
    }

}

BOOST_AUTO_TEST_SUITE_END()
