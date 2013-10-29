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

BOOST_AUTO_TEST_CASE(insert_delete)
{
    // Test inserting/deleting bytes.
    CDataStream ss(SER_DISK, 0);
    BOOST_CHECK_EQUAL(ss.size(), 0);

    ss.write("\x00\x01\x02\xff", 4);
    BOOST_CHECK_EQUAL(ss.size(), 4);

    char c = (char)11;

    // Inserting at beginning/end/middle:
    ss.insert(ss.begin(), c);
    BOOST_CHECK_EQUAL(ss.size(), 5);
    BOOST_CHECK_EQUAL(ss[0], c);
    BOOST_CHECK_EQUAL(ss[1], 0);

    ss.insert(ss.end(), c);
    BOOST_CHECK_EQUAL(ss.size(), 6);
    BOOST_CHECK_EQUAL(ss[4], (char)0xff);
    BOOST_CHECK_EQUAL(ss[5], c);

    ss.insert(ss.begin()+2, c);
    BOOST_CHECK_EQUAL(ss.size(), 7);
    BOOST_CHECK_EQUAL(ss[2], c);

    // Delete at beginning/end/middle
    ss.erase(ss.begin());
    BOOST_CHECK_EQUAL(ss.size(), 6);
    BOOST_CHECK_EQUAL(ss[0], 0);

    ss.erase(ss.begin()+ss.size()-1);
    BOOST_CHECK_EQUAL(ss.size(), 5);
    BOOST_CHECK_EQUAL(ss[4], (char)0xff);

    ss.erase(ss.begin()+1);
    BOOST_CHECK_EQUAL(ss.size(), 4);
    BOOST_CHECK_EQUAL(ss[0], 0);
    BOOST_CHECK_EQUAL(ss[1], 1);
    BOOST_CHECK_EQUAL(ss[2], 2);
    BOOST_CHECK_EQUAL(ss[3], (char)0xff);

    // Make sure GetAndClear does the right thing:
    CSerializeData d;
    ss.GetAndClear(d);
    BOOST_CHECK_EQUAL(ss.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END()
