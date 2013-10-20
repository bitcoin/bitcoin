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

BOOST_AUTO_TEST_CASE(compactsize)
{
    CDataStream ss(SER_DISK, 0);
    vector<char>::size_type i, j;

    for (i = 1; i <= MAX_SIZE; i *= 2)
    {
        WriteCompactSize(ss, i-1);
        WriteCompactSize(ss, i);
    }
    for (i = 1; i <= MAX_SIZE; i *= 2)
    {
        j = ReadCompactSize(ss);
        BOOST_CHECK_MESSAGE((i-1) == j, "decoded:" << j << " expected:" << (i-1));
        j = ReadCompactSize(ss);
        BOOST_CHECK_MESSAGE(i == j, "decoded:" << j << " expected:" << i);
    }
}

static bool isCanonicalException(const std::ios_base::failure& ex)
{
    return std::string("non-canonical ReadCompactSize()") == ex.what();
}

BOOST_AUTO_TEST_CASE(noncanonical)
{
    // Write some non-canonical CompactSize encodings, and
    // make sure an exception is thrown when read back.
    CDataStream ss(SER_DISK, 0);
    vector<char>::size_type n;

    // zero encoded with three bytes:
    ss.write("\xfd\x00\x00", 3);
    BOOST_CHECK_EXCEPTION(ReadCompactSize(ss), std::ios_base::failure, isCanonicalException);

    // 0xfc encoded with three bytes:
    ss.write("\xfd\xfc\x00", 3);
    BOOST_CHECK_EXCEPTION(ReadCompactSize(ss), std::ios_base::failure, isCanonicalException);

    // 0xfd encoded with three bytes is OK:
    ss.write("\xfd\xfd\x00", 3);
    n = ReadCompactSize(ss);
    BOOST_CHECK(n == 0xfd);

    // zero encoded with five bytes:
    ss.write("\xfe\x00\x00\x00\x00", 5);
    BOOST_CHECK_EXCEPTION(ReadCompactSize(ss), std::ios_base::failure, isCanonicalException);

    // 0xffff encoded with five bytes:
    ss.write("\xfe\xff\xff\x00\x00", 5);
    BOOST_CHECK_EXCEPTION(ReadCompactSize(ss), std::ios_base::failure, isCanonicalException);

    // zero encoded with nine bytes:
    ss.write("\xff\x00\x00\x00\x00\x00\x00\x00\x00", 9);
    BOOST_CHECK_EXCEPTION(ReadCompactSize(ss), std::ios_base::failure, isCanonicalException);

    // 0x01ffffff encoded with nine bytes:
    ss.write("\xff\xff\xff\xff\x01\x00\x00\x00\x00", 9);
    BOOST_CHECK_EXCEPTION(ReadCompactSize(ss), std::ios_base::failure, isCanonicalException);
}

BOOST_AUTO_TEST_SUITE_END()
