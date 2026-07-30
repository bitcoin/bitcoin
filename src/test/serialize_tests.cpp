// Copyright (c) 2012-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <hash.h>
#include <serialize.h>
#include <streams.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>

#include <stdint.h>

#include <limits>
#include <stdexcept>
#include <string_view>
#include <vector>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(serialize_tests, BasicTestingSetup)

class CSerializeMethodsTestSingle
{
protected:
    int intval;
    bool boolval;
    std::string stringval;
    char charstrval[16];
    CTransactionRef txval;
public:
    CSerializeMethodsTestSingle() = default;
    CSerializeMethodsTestSingle(int intvalin, bool boolvalin, std::string stringvalin, const uint8_t* charstrvalin, const CTransactionRef& txvalin) : intval(intvalin), boolval(boolvalin), stringval(std::move(stringvalin)), txval(txvalin)
    {
        memcpy(charstrval, charstrvalin, sizeof(charstrval));
    }

    SERIALIZE_METHODS(CSerializeMethodsTestSingle, obj)
    {
        READWRITE(obj.intval);
        READWRITE(obj.boolval);
        READWRITE(obj.stringval);
        READWRITE(obj.charstrval);
        READWRITE(obj.txval);
    }

    bool operator==(const CSerializeMethodsTestSingle& rhs) const
    {
        return intval == rhs.intval &&
               boolval == rhs.boolval &&
               stringval == rhs.stringval &&
               strcmp(charstrval, rhs.charstrval) == 0 &&
               *txval == *rhs.txval;
    }
};

class CSerializeMethodsTestMany : public CSerializeMethodsTestSingle
{
public:
    using CSerializeMethodsTestSingle::CSerializeMethodsTestSingle;

    SERIALIZE_METHODS(CSerializeMethodsTestMany, obj)
    {
        READWRITE(obj.intval, obj.boolval, obj.stringval, obj.charstrval, obj.txval);
    }
};

BOOST_AUTO_TEST_CASE(sizes)
{
    BOOST_CHECK_EQUAL(sizeof(unsigned char), GetSerializeSize((unsigned char)0, 0));
    BOOST_CHECK_EQUAL(sizeof(int8_t), GetSerializeSize(int8_t(0), 0));
    BOOST_CHECK_EQUAL(sizeof(uint8_t), GetSerializeSize(uint8_t(0), 0));
    BOOST_CHECK_EQUAL(sizeof(int16_t), GetSerializeSize(int16_t(0), 0));
    BOOST_CHECK_EQUAL(sizeof(uint16_t), GetSerializeSize(uint16_t(0), 0));
    BOOST_CHECK_EQUAL(sizeof(int32_t), GetSerializeSize(int32_t(0), 0));
    BOOST_CHECK_EQUAL(sizeof(uint32_t), GetSerializeSize(uint32_t(0), 0));
    BOOST_CHECK_EQUAL(sizeof(int64_t), GetSerializeSize(int64_t(0), 0));
    BOOST_CHECK_EQUAL(sizeof(uint64_t), GetSerializeSize(uint64_t(0), 0));
    // Bool is serialized as uint8_t
    BOOST_CHECK_EQUAL(sizeof(uint8_t), GetSerializeSize(bool(0), 0));

    // Sanity-check GetSerializeSize and c++ type matching
    BOOST_CHECK_EQUAL(GetSerializeSize((unsigned char)0, 0), 1U);
    BOOST_CHECK_EQUAL(GetSerializeSize(int8_t(0), 0), 1U);
    BOOST_CHECK_EQUAL(GetSerializeSize(uint8_t(0), 0), 1U);
    BOOST_CHECK_EQUAL(GetSerializeSize(int16_t(0), 0), 2U);
    BOOST_CHECK_EQUAL(GetSerializeSize(uint16_t(0), 0), 2U);
    BOOST_CHECK_EQUAL(GetSerializeSize(int32_t(0), 0), 4U);
    BOOST_CHECK_EQUAL(GetSerializeSize(uint32_t(0), 0), 4U);
    BOOST_CHECK_EQUAL(GetSerializeSize(int64_t(0), 0), 8U);
    BOOST_CHECK_EQUAL(GetSerializeSize(uint64_t(0), 0), 8U);
    BOOST_CHECK_EQUAL(GetSerializeSize(bool(0), 0), 1U);
}

BOOST_AUTO_TEST_CASE(varints)
{
    // encode

    CDataStream ss(SER_DISK, 0);
    CDataStream::size_type size = 0;
    for (int i = 0; i < 100000; i++) {
        ss << VARINT_MODE(i, VarIntMode::NONNEGATIVE_SIGNED);
        size += ::GetSerializeSize(VARINT_MODE(i, VarIntMode::NONNEGATIVE_SIGNED), 0);
        BOOST_CHECK(size == ss.size());
    }

    for (uint64_t i = 0;  i < 100000000000ULL; i += 999999937) {
        ss << VARINT(i);
        size += ::GetSerializeSize(VARINT(i), 0);
        BOOST_CHECK(size == ss.size());
    }

    // decode
    for (int i = 0; i < 100000; i++) {
        int j = -1;
        ss >> VARINT_MODE(j, VarIntMode::NONNEGATIVE_SIGNED);
        BOOST_CHECK_MESSAGE(i == j, "decoded:" << j << " expected:" << i);
    }

    for (uint64_t i = 0;  i < 100000000000ULL; i += 999999937) {
        uint64_t j = std::numeric_limits<uint64_t>::max();
        ss >> VARINT(j);
        BOOST_CHECK_MESSAGE(i == j, "decoded:" << j << " expected:" << i);
    }
}

BOOST_AUTO_TEST_CASE(varints_bitpatterns)
{
    CDataStream ss(SER_DISK, 0);
    ss << VARINT_MODE(0, VarIntMode::NONNEGATIVE_SIGNED); BOOST_CHECK_EQUAL(HexStr(ss), "00"); ss.clear();
    ss << VARINT_MODE(0x7f, VarIntMode::NONNEGATIVE_SIGNED); BOOST_CHECK_EQUAL(HexStr(ss), "7f"); ss.clear();
    ss << VARINT_MODE(int8_t{0x7f}, VarIntMode::NONNEGATIVE_SIGNED); BOOST_CHECK_EQUAL(HexStr(ss), "7f"); ss.clear();
    ss << VARINT_MODE(0x80, VarIntMode::NONNEGATIVE_SIGNED); BOOST_CHECK_EQUAL(HexStr(ss), "8000"); ss.clear();
    ss << VARINT(uint8_t{0x80}); BOOST_CHECK_EQUAL(HexStr(ss), "8000"); ss.clear();
    ss << VARINT_MODE(0x1234, VarIntMode::NONNEGATIVE_SIGNED); BOOST_CHECK_EQUAL(HexStr(ss), "a334"); ss.clear();
    ss << VARINT_MODE(int16_t{0x1234}, VarIntMode::NONNEGATIVE_SIGNED); BOOST_CHECK_EQUAL(HexStr(ss), "a334"); ss.clear();
    ss << VARINT_MODE(0xffff, VarIntMode::NONNEGATIVE_SIGNED); BOOST_CHECK_EQUAL(HexStr(ss), "82fe7f"); ss.clear();
    ss << VARINT(uint16_t{0xffff}); BOOST_CHECK_EQUAL(HexStr(ss), "82fe7f"); ss.clear();
    ss << VARINT_MODE(0x123456, VarIntMode::NONNEGATIVE_SIGNED); BOOST_CHECK_EQUAL(HexStr(ss), "c7e756"); ss.clear();
    ss << VARINT_MODE(int32_t{0x123456}, VarIntMode::NONNEGATIVE_SIGNED); BOOST_CHECK_EQUAL(HexStr(ss), "c7e756"); ss.clear();
    ss << VARINT(0x80123456U); BOOST_CHECK_EQUAL(HexStr(ss), "86ffc7e756"); ss.clear();
    ss << VARINT(uint32_t{0x80123456U}); BOOST_CHECK_EQUAL(HexStr(ss), "86ffc7e756"); ss.clear();
    ss << VARINT(0xffffffff); BOOST_CHECK_EQUAL(HexStr(ss), "8efefefe7f"); ss.clear();
    ss << VARINT_MODE(0x7fffffffffffffffLL, VarIntMode::NONNEGATIVE_SIGNED); BOOST_CHECK_EQUAL(HexStr(ss), "fefefefefefefefe7f"); ss.clear();
    ss << VARINT(0xffffffffffffffffULL); BOOST_CHECK_EQUAL(HexStr(ss), "80fefefefefefefefe7f"); ss.clear();
}

BOOST_AUTO_TEST_CASE(compactsize)
{
    CDataStream ss(SER_DISK, 0);
    std::vector<char>::size_type i, j;

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
    std::ios_base::failure expectedException("non-canonical ReadCompactSize()");

    // The string returned by what() can be different for different platforms.
    // Instead of directly comparing the ex.what() with an expected string,
    // create an instance of exception to see if ex.what() matches
    // the expected explanatory string returned by the exception instance.
    return strcmp(expectedException.what(), ex.what()) == 0;
}

BOOST_AUTO_TEST_CASE(vector_bool)
{
    std::vector<uint8_t> vec1{1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 1};
    std::vector<bool> vec2{1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 1};

    BOOST_CHECK(vec1 == std::vector<uint8_t>(vec2.begin(), vec2.end()));
    BOOST_CHECK(SerializeHash(vec1) == SerializeHash(vec2));
}

BOOST_AUTO_TEST_CASE(noncanonical)
{
    // Write some non-canonical CompactSize encodings, and
    // make sure an exception is thrown when read back.
    CDataStream ss(SER_DISK, 0);
    std::vector<char>::size_type n;

    // zero encoded with three bytes:
    ss.write(MakeByteSpan("\xfd\x00\x00").first(3));
    BOOST_CHECK_EXCEPTION(ReadCompactSize(ss), std::ios_base::failure, isCanonicalException);

    // 0xfc encoded with three bytes:
    ss.write(MakeByteSpan("\xfd\xfc\x00").first(3));
    BOOST_CHECK_EXCEPTION(ReadCompactSize(ss), std::ios_base::failure, isCanonicalException);

    // 0xfd encoded with three bytes is OK:
    ss.write(MakeByteSpan("\xfd\xfd\x00").first(3));
    n = ReadCompactSize(ss);
    BOOST_CHECK(n == 0xfd);

    // zero encoded with five bytes:
    ss.write(MakeByteSpan("\xfe\x00\x00\x00\x00").first(5));
    BOOST_CHECK_EXCEPTION(ReadCompactSize(ss), std::ios_base::failure, isCanonicalException);

    // 0xffff encoded with five bytes:
    ss.write(MakeByteSpan("\xfe\xff\xff\x00\x00").first(5));
    BOOST_CHECK_EXCEPTION(ReadCompactSize(ss), std::ios_base::failure, isCanonicalException);

    // zero encoded with nine bytes:
    ss.write(MakeByteSpan("\xff\x00\x00\x00\x00\x00\x00\x00\x00").first(9));
    BOOST_CHECK_EXCEPTION(ReadCompactSize(ss), std::ios_base::failure, isCanonicalException);

    // 0x01ffffff encoded with nine bytes:
    ss.write(MakeByteSpan("\xff\xff\xff\xff\x01\x00\x00\x00\x00").first(9));
    BOOST_CHECK_EXCEPTION(ReadCompactSize(ss), std::ios_base::failure, isCanonicalException);
}

// Change struct size and check if it can be deserialized
// from old version archive and vice versa
struct old_version
{
    int field1;

    SERIALIZE_METHODS(old_version, obj)
    {
        READWRITE(obj.field1);
    }
};

struct new_version
{
    int field1;
    int field2;

    template<typename Stream>
    void Serialize(Stream &s) const
    {
        s << field1 << field2;
    }

    template<typename Stream>
    void Unserialize(Stream &s)
    {
        s >> field1;
        if (s.size() == 0) {
            field2 = 0;
            return;
        }
        s >> field2;
    }
};

BOOST_AUTO_TEST_CASE(check_backward_compatibility)
{
    CDataStream ss(SER_DISK, 0);
    old_version old_src({5});
    ss << old_src;
    new_version new_dest({6, 7});
    BOOST_REQUIRE_NO_THROW(ss >> new_dest);
    BOOST_REQUIRE(old_src.field1 == new_dest.field1);
    BOOST_REQUIRE(ss.size() == 0);

    new_version new_src({6, 7});
    ss << new_src;
    old_version old_dest({5});
    BOOST_REQUIRE_NO_THROW(ss >> old_dest);
    BOOST_REQUIRE(new_src.field1 == old_dest.field1);
}

BOOST_AUTO_TEST_CASE(class_methods)
{
    int intval(100);
    bool boolval(true);
    std::string stringval("testing");
    const uint8_t charstrval[16]{"testing charstr"};
    CMutableTransaction txval;
    CTransactionRef tx_ref{MakeTransactionRef(txval)};
    CSerializeMethodsTestSingle methodtest1(intval, boolval, stringval, charstrval, tx_ref);
    CSerializeMethodsTestMany methodtest2(intval, boolval, stringval, charstrval, tx_ref);
    CSerializeMethodsTestSingle methodtest3;
    CSerializeMethodsTestMany methodtest4;
    CDataStream ss(SER_DISK, PROTOCOL_VERSION);
    BOOST_CHECK(methodtest1 == methodtest2);
    ss << methodtest1;
    ss >> methodtest4;
    ss << methodtest2;
    ss >> methodtest3;
    BOOST_CHECK(methodtest1 == methodtest2);
    BOOST_CHECK(methodtest2 == methodtest3);
    BOOST_CHECK(methodtest3 == methodtest4);

    CDataStream ss2{SER_DISK, PROTOCOL_VERSION};
    ss2 << intval << boolval << stringval << charstrval << txval;
    ss2 >> methodtest3;
    BOOST_CHECK(methodtest3 == methodtest4);
    {
        DataStream ds;
        const std::string in{"ab"};
        ds << Span{in} << std::byte{'c'};
        std::array<std::byte, 2> out;
        std::byte out_3;
        ds >> Span{out} >> out_3;
        BOOST_CHECK_EQUAL(out.at(0), std::byte{'a'});
        BOOST_CHECK_EQUAL(out.at(1), std::byte{'b'});
        BOOST_CHECK_EQUAL(out_3, std::byte{'c'});
    }
}

namespace {
// Element formatter that counts Unser invocations, so a test can prove the
// bounded reader rejected an oversized wire count *before* touching any
// element. Ser delegates to the default Serialize so wire compatibility is
// preserved.
struct CountingFormatter
{
    static inline size_t unser_calls = 0;
    static void Reset() { unser_calls = 0; }

    template<typename Stream, typename T>
    void Ser(Stream& s, const T& t) { Serialize(s, t); }

    template<typename Stream, typename T>
    void Unser(Stream& s, T& t)
    {
        ++unser_calls;
        Unserialize(s, t);
    }
};

bool IsLimitExceededFailure(const std::ios_base::failure& e)
{
    return std::string_view{e.what()}.find("Vector length limit exceeded") != std::string_view::npos;
}

// Struct with a compile-time bounded vector member, so the compile-time
// formatter can be exercised end-to-end via ordinary << / >> and READWRITE.
struct LimitedVecFive
{
    std::vector<int> v;
    SERIALIZE_METHODS(LimitedVecFive, obj) { READWRITE(LIMITED_VECTOR(obj.v, 5)); }
};
} // namespace

BOOST_AUTO_TEST_CASE(unserialize_vector_with_max_size)
{
    // Within-limit round trip: an ordinary vector encoding decodes cleanly and
    // the wire bytes are byte-identical to the unbounded writer's output, so
    // this helper stays interchangeable with the standard vector Unserialize.
    {
        DataStream ss;
        const std::vector<int> v{1, 2, 3};
        ss << v;

        DataStream ref;
        ref << v;
        BOOST_CHECK_EQUAL_COLLECTIONS(ss.begin(), ss.end(), ref.begin(), ref.end());

        std::vector<int> out;
        BOOST_CHECK(UnserializeVectorWithMaxSize(ss, out, /*max_size=*/8));
        BOOST_CHECK(out == v);
        BOOST_CHECK_EQUAL(ss.size(), 0U);
    }

    // Zero limit rejects any non-empty vector and accepts an empty one.
    {
        DataStream ss;
        ss << std::vector<int>{};
        std::vector<int> out{99};
        BOOST_CHECK(UnserializeVectorWithMaxSize(ss, out, /*max_size=*/0));
        BOOST_CHECK(out.empty());
    }
    {
        DataStream ss;
        ss << std::vector<int>{1};
        std::vector<int> out;
        BOOST_CHECK(!UnserializeVectorWithMaxSize(ss, out, /*max_size=*/0));
        BOOST_CHECK(out.empty());
    }

    // Exact-limit boundary decodes; one-over boundary is rejected and no
    // element decoding happens.
    {
        DataStream ss;
        const std::vector<int> v{10, 20, 30, 40};
        ss << v;
        std::vector<int> out;
        BOOST_CHECK(UnserializeVectorWithMaxSize(ss, out, /*max_size=*/4));
        BOOST_CHECK(out == v);
    }
    {
        DataStream ss;
        const std::vector<int> v{10, 20, 30, 40, 50};
        ss << v;
        std::vector<int> out;
        CountingFormatter::Reset();
        BOOST_CHECK(!UnserializeVectorWithMaxSize<CountingFormatter>(ss, out, /*max_size=*/4));
        BOOST_CHECK(out.empty());
        BOOST_CHECK_EQUAL(CountingFormatter::unser_calls, 0U);
    }

    // Custom element formatter path: within-limit, verify the element formatter
    // actually runs; over-limit, verify it does not.
    {
        DataStream ss;
        ss << std::vector<int>{7, 8, 9};
        std::vector<int> out;
        CountingFormatter::Reset();
        BOOST_CHECK(UnserializeVectorWithMaxSize<CountingFormatter>(ss, out, /*max_size=*/8));
        BOOST_CHECK_EQUAL(CountingFormatter::unser_calls, 3U);
        BOOST_CHECK((out == std::vector<int>{7, 8, 9}));
    }

    // A wire count at MAX_SIZE still hits the caller's own limit (8) first and
    // returns false without decoding any element.
    {
        static_assert(MAX_SIZE < std::numeric_limits<uint32_t>::max(),
                      "MAX_SIZE must fit in a 32-bit CompactSize prefix for this test");
        DataStream ss;
        ss << uint8_t{0xfe};
        ss << static_cast<uint32_t>(MAX_SIZE);
        std::vector<int> out;
        CountingFormatter::Reset();
        BOOST_CHECK(!UnserializeVectorWithMaxSize<CountingFormatter>(ss, out, /*max_size=*/8));
        BOOST_CHECK(out.empty());
        BOOST_CHECK_EQUAL(CountingFormatter::unser_calls, 0U);
    }

    // Counts above MAX_SIZE throw from ReadCompactSize before the caller's
    // gate is consulted — these are malformed at the CompactSize level.
    {
        DataStream ss;
        ss << uint8_t{0xfe};
        ss << static_cast<uint32_t>(MAX_SIZE + 1);
        std::vector<int> out;
        BOOST_CHECK_THROW((void)UnserializeVectorWithMaxSize<CountingFormatter>(ss, out, /*max_size=*/8),
                          std::ios_base::failure);
    }
    {
        DataStream ss;
        ss << uint8_t{0xff};
        ss << uint64_t{0x100000000ULL + MAX_SIZE};
        std::vector<int> out;
        BOOST_CHECK_THROW((void)UnserializeVectorWithMaxSize<CountingFormatter>(ss, out, /*max_size=*/8),
                          std::ios_base::failure);
    }
}

BOOST_AUTO_TEST_CASE(limited_vector_formatter_compile_time)
{
    // The compile-time formatter's wire format matches the ordinary vector
    // encoding — a struct wrapping std::vector<int> under LIMITED_VECTOR must
    // produce the same bytes as the raw vector.
    {
        DataStream ss_lim;
        DataStream ss_ord;
        LimitedVecFive obj{{1, 2, 3}};
        ss_lim << obj;
        ss_ord << obj.v;
        BOOST_CHECK_EQUAL_COLLECTIONS(ss_lim.begin(), ss_lim.end(),
                                      ss_ord.begin(), ss_ord.end());

        LimitedVecFive round;
        ss_lim >> round;
        BOOST_CHECK(round.v == obj.v);
    }

    // At the exact limit: round-trips.
    {
        DataStream ss;
        LimitedVecFive obj{{1, 2, 3, 4, 5}};
        ss << obj;
        LimitedVecFive round;
        BOOST_REQUIRE_NO_THROW(ss >> round);
        BOOST_CHECK(round.v == obj.v);
    }

    // Over the limit: rejected with the sentinel failure, before element decode.
    {
        DataStream ss;
        ss << std::vector<int>{1, 2, 3, 4, 5, 6};
        LimitedVecFive round;
        BOOST_CHECK_EXCEPTION(ss >> round, std::ios_base::failure, IsLimitExceededFailure);
        BOOST_CHECK(round.v.empty());
    }

    // Serialization stays unbounded: writing an over-limit vector through the
    // formatter must still emit the ordinary wire format, so producers remain
    // compatible with the standard reader. Reading it back through the
    // unbounded std::vector path succeeds even though the LIMITED_VECTOR reader
    // would reject it.
    {
        DataStream ss;
        const std::vector<int> big{1, 2, 3, 4, 5, 6, 7};
        ss << Using<LimitedVectorFormatter<5>>(big);

        DataStream ref;
        ref << big;
        BOOST_CHECK_EQUAL_COLLECTIONS(ss.begin(), ss.end(), ref.begin(), ref.end());

        std::vector<int> out;
        BOOST_REQUIRE_NO_THROW(ss >> out);
        BOOST_CHECK(out == big);
    }
}

BOOST_AUTO_TEST_SUITE_END()
