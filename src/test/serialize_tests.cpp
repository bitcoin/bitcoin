// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <hash.h>
#include <serialize.h>
#include <streams.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>

#include <stdint.h>
#include <string>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(serialize_tests, BasicTestingSetup)

// For testing move-semantics, declare a version of datastream that can be moved
// but is not copyable.
class UncopyableStream : public DataStream
{
public:
    using DataStream::DataStream;
    UncopyableStream(const UncopyableStream&) = delete;
    UncopyableStream& operator=(const UncopyableStream&) = delete;
    UncopyableStream(UncopyableStream&&) noexcept = default;
    UncopyableStream& operator=(UncopyableStream&&) noexcept = default;
};

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
        READWRITE(TX_WITH_WITNESS(obj.txval));
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
        READWRITE(obj.intval, obj.boolval, obj.stringval, obj.charstrval, TX_WITH_WITNESS(obj.txval));
    }
};

BOOST_AUTO_TEST_CASE(sizes)
{
    BOOST_CHECK_EQUAL(sizeof(unsigned char), GetSerializeSize((unsigned char)0));
    BOOST_CHECK_EQUAL(sizeof(int8_t), GetSerializeSize(int8_t(0)));
    BOOST_CHECK_EQUAL(sizeof(uint8_t), GetSerializeSize(uint8_t(0)));
    BOOST_CHECK_EQUAL(sizeof(int16_t), GetSerializeSize(int16_t(0)));
    BOOST_CHECK_EQUAL(sizeof(uint16_t), GetSerializeSize(uint16_t(0)));
    BOOST_CHECK_EQUAL(sizeof(int32_t), GetSerializeSize(int32_t(0)));
    BOOST_CHECK_EQUAL(sizeof(uint32_t), GetSerializeSize(uint32_t(0)));
    BOOST_CHECK_EQUAL(sizeof(int64_t), GetSerializeSize(int64_t(0)));
    BOOST_CHECK_EQUAL(sizeof(uint64_t), GetSerializeSize(uint64_t(0)));
    // Bool is serialized as uint8_t
    BOOST_CHECK_EQUAL(sizeof(uint8_t), GetSerializeSize(bool(0)));

    // Sanity-check GetSerializeSize and c++ type matching
    BOOST_CHECK_EQUAL(GetSerializeSize((unsigned char)0), 1U);
    BOOST_CHECK_EQUAL(GetSerializeSize(int8_t(0)), 1U);
    BOOST_CHECK_EQUAL(GetSerializeSize(uint8_t(0)), 1U);
    BOOST_CHECK_EQUAL(GetSerializeSize(int16_t(0)), 2U);
    BOOST_CHECK_EQUAL(GetSerializeSize(uint16_t(0)), 2U);
    BOOST_CHECK_EQUAL(GetSerializeSize(int32_t(0)), 4U);
    BOOST_CHECK_EQUAL(GetSerializeSize(uint32_t(0)), 4U);
    BOOST_CHECK_EQUAL(GetSerializeSize(int64_t(0)), 8U);
    BOOST_CHECK_EQUAL(GetSerializeSize(uint64_t(0)), 8U);
    BOOST_CHECK_EQUAL(GetSerializeSize(bool(0)), 1U);
    BOOST_CHECK_EQUAL(GetSerializeSize(std::array<uint8_t, 1>{0}), 1U);
    BOOST_CHECK_EQUAL(GetSerializeSize(std::array<uint8_t, 2>{0, 0}), 2U);
}

BOOST_AUTO_TEST_CASE(varints)
{
    // encode

    DataStream ss{};
    DataStream::size_type size = 0;
    for (int i = 0; i < 100000; i++) {
        ss << VARINT_MODE(i, VarIntMode::NONNEGATIVE_SIGNED);
        size += ::GetSerializeSize(VARINT_MODE(i, VarIntMode::NONNEGATIVE_SIGNED));
        BOOST_CHECK(size == ss.size());
    }

    for (uint64_t i = 0;  i < 100000000000ULL; i += 999999937) {
        ss << VARINT(i);
        size += ::GetSerializeSize(VARINT(i));
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
    DataStream ss{};
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
    DataStream ss{};
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
    BOOST_CHECK((HashWriter{} << vec1).GetHash() == (HashWriter{} << vec2).GetHash());
}

BOOST_AUTO_TEST_CASE(array)
{
    std::array<uint8_t, 32> array1{1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 1};
    DataStream ds;
    ds << array1;
    std::array<uint8_t, 32> array2;
    ds >> array2;
    BOOST_CHECK(array1 == array2);
}

BOOST_AUTO_TEST_CASE(noncanonical)
{
    // Write some non-canonical CompactSize encodings, and
    // make sure an exception is thrown when read back.
    DataStream ss{};
    std::vector<char>::size_type n;

    // zero encoded with three bytes:
    ss << std::span{"\xfd\x00\x00"}.first(3);
    BOOST_CHECK_EXCEPTION(ReadCompactSize(ss), std::ios_base::failure, isCanonicalException);

    // 0xfc encoded with three bytes:
    ss << std::span{"\xfd\xfc\x00"}.first(3);
    BOOST_CHECK_EXCEPTION(ReadCompactSize(ss), std::ios_base::failure, isCanonicalException);

    // 0xfd encoded with three bytes is OK:
    ss << std::span{"\xfd\xfd\x00"}.first(3);
    n = ReadCompactSize(ss);
    BOOST_CHECK(n == 0xfd);

    // zero encoded with five bytes:
    ss << std::span{"\xfe\x00\x00\x00\x00"}.first(5);
    BOOST_CHECK_EXCEPTION(ReadCompactSize(ss), std::ios_base::failure, isCanonicalException);

    // 0xffff encoded with five bytes:
    ss << std::span{"\xfe\xff\xff\x00\x00"}.first(5);
    BOOST_CHECK_EXCEPTION(ReadCompactSize(ss), std::ios_base::failure, isCanonicalException);

    // zero encoded with nine bytes:
    ss << std::span{"\xff\x00\x00\x00\x00\x00\x00\x00\x00"}.first(9);
    BOOST_CHECK_EXCEPTION(ReadCompactSize(ss), std::ios_base::failure, isCanonicalException);

    // 0x01ffffff encoded with nine bytes:
    ss << std::span{"\xff\xff\xff\xff\x01\x00\x00\x00\x00"}.first(9);
    BOOST_CHECK_EXCEPTION(ReadCompactSize(ss), std::ios_base::failure, isCanonicalException);
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
    DataStream ss;
    BOOST_CHECK(methodtest1 == methodtest2);
    ss << methodtest1;
    ss >> methodtest4;
    ss << methodtest2;
    ss >> methodtest3;
    BOOST_CHECK(methodtest1 == methodtest2);
    BOOST_CHECK(methodtest2 == methodtest3);
    BOOST_CHECK(methodtest3 == methodtest4);

    DataStream ss2;
    ss2 << intval << boolval << stringval << charstrval << TX_WITH_WITNESS(txval);
    ss2 >> methodtest3;
    BOOST_CHECK(methodtest3 == methodtest4);
    {
        DataStream ds;
        const std::string in{"ab"};
        ds << std::span{in} << std::byte{'c'};
        std::array<std::byte, 2> out;
        std::byte out_3;
        ds >> std::span{out} >> out_3;
        BOOST_CHECK_EQUAL(out.at(0), std::byte{'a'});
        BOOST_CHECK_EQUAL(out.at(1), std::byte{'b'});
        BOOST_CHECK_EQUAL(out_3, std::byte{'c'});
    }
}

struct BaseFormat {
    const enum {
        RAW,
        HEX,
    } m_base_format;
    SER_PARAMS_OPFUNC
};
constexpr BaseFormat RAW{BaseFormat::RAW};
constexpr BaseFormat HEX{BaseFormat::HEX};

/// (Un)serialize a number as raw byte or 2 hexadecimal chars.
class Base
{
public:
    uint8_t m_base_data;

    Base() : m_base_data(17) {}
    explicit Base(uint8_t data) : m_base_data(data) {}

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        if (s.template GetParams<BaseFormat>().m_base_format == BaseFormat::RAW) {
            s << m_base_data;
        } else {
            s << std::span<const char>{HexStr(std::span{&m_base_data, 1})};
        }
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        if (s.template GetParams<BaseFormat>().m_base_format == BaseFormat::RAW) {
            s >> m_base_data;
        } else {
            std::string hex{"aa"};
            s >> std::span{hex}.first(hex.size());
            m_base_data = TryParseHex<uint8_t>(hex).value().at(0);
        }
    }
};

class DerivedAndBaseFormat
{
public:
    BaseFormat m_base_format;

    enum class DerivedFormat {
        LOWER,
        UPPER,
    } m_derived_format;

    SER_PARAMS_OPFUNC
};

class Derived : public Base
{
public:
    std::string m_derived_data;

    SERIALIZE_METHODS(Derived, obj)
    {
        auto& fmt = SER_PARAMS(DerivedAndBaseFormat);
        READWRITE(fmt.m_base_format(AsBase<Base>(obj)));

        if (ser_action.ForRead()) {
            std::string str;
            s >> str;
            SER_READ(obj, obj.m_derived_data = str);
        } else {
            s << (fmt.m_derived_format == DerivedAndBaseFormat::DerivedFormat::LOWER ?
                      ToLower(obj.m_derived_data) :
                      ToUpper(obj.m_derived_data));
        }
    }
};

struct OtherParam {
    uint8_t param;
    SER_PARAMS_OPFUNC
};

//! Checker for value of OtherParam. When being serialized, serializes the
//! param to the stream. When being unserialized, verifies the value in the
//! stream matches the param.
class OtherParamChecker
{
public:
    template <typename Stream>
    void Serialize(Stream& s) const
    {
        const uint8_t param = s.template GetParams<OtherParam>().param;
        s << param;
    }

    template <typename Stream>
    void Unserialize(Stream& s) const
    {
        const uint8_t param = s.template GetParams<OtherParam>().param;
        uint8_t value;
        s >> value;
        BOOST_CHECK_EQUAL(value, param);
    }
};

//! Test creating a stream with multiple parameters and making sure
//! serialization code requiring different parameters can retrieve them. Also
//! test that earlier parameters take precedence if the same parameter type is
//! specified twice. (Choice of whether earlier or later values take precedence
//! or multiple values of the same type are allowed was arbitrary, and just
//! decided based on what would require smallest amount of ugly C++ template
//! code. Intent of the test is to just ensure there is no unexpected behavior.)
BOOST_AUTO_TEST_CASE(with_params_multi)
{
    const OtherParam other_param_used{.param = 0x10};
    const OtherParam other_param_ignored{.param = 0x11};
    const OtherParam other_param_override{.param = 0x12};
    const OtherParamChecker check;
    DataStream stream;
    ParamsStream pstream{stream, RAW, other_param_used, other_param_ignored};

    Base base1{0x20};
    pstream << base1 << check << other_param_override(check);
    BOOST_CHECK_EQUAL(stream.str(), "\x20\x10\x12");

    Base base2;
    pstream >> base2 >> check >> other_param_override(check);
    BOOST_CHECK_EQUAL(base2.m_base_data, 0x20);
}

//! Test creating a ParamsStream that moves from a stream argument.
BOOST_AUTO_TEST_CASE(with_params_move)
{
    UncopyableStream stream{MakeByteSpan(std::string_view{"abc"})};
    ParamsStream pstream{std::move(stream), RAW, HEX, RAW};
    BOOST_CHECK_EQUAL(pstream.GetStream().str(), "abc");
    pstream.GetStream().clear();

    Base base1{0x20};
    pstream << base1;
    BOOST_CHECK_EQUAL(pstream.GetStream().str(), "\x20");

    Base base2;
    pstream >> base2;
    BOOST_CHECK_EQUAL(base2.m_base_data, 0x20);
}

BOOST_AUTO_TEST_CASE(with_params_base)
{
    Base b{0x0F};

    DataStream stream;

    stream << RAW(b);
    BOOST_CHECK_EQUAL(stream.str(), "\x0F");

    b.m_base_data = 0;
    stream >> RAW(b);
    BOOST_CHECK_EQUAL(b.m_base_data, 0x0F);

    stream.clear();

    stream << HEX(b);
    BOOST_CHECK_EQUAL(stream.str(), "0f");

    b.m_base_data = 0;
    stream >> HEX(b);
    BOOST_CHECK_EQUAL(b.m_base_data, 0x0F);
}

BOOST_AUTO_TEST_CASE(with_params_vector_of_base)
{
    std::vector<Base> v{Base{0x0F}, Base{0xFF}};

    DataStream stream;

    stream << RAW(v);
    BOOST_CHECK_EQUAL(stream.str(), "\x02\x0F\xFF");

    v[0].m_base_data = 0;
    v[1].m_base_data = 0;
    stream >> RAW(v);
    BOOST_CHECK_EQUAL(v[0].m_base_data, 0x0F);
    BOOST_CHECK_EQUAL(v[1].m_base_data, 0xFF);

    stream.clear();

    stream << HEX(v);
    BOOST_CHECK_EQUAL(stream.str(), "\x02"
                                    "0fff");

    v[0].m_base_data = 0;
    v[1].m_base_data = 0;
    stream >> HEX(v);
    BOOST_CHECK_EQUAL(v[0].m_base_data, 0x0F);
    BOOST_CHECK_EQUAL(v[1].m_base_data, 0xFF);
}

constexpr DerivedAndBaseFormat RAW_LOWER{{BaseFormat::RAW}, DerivedAndBaseFormat::DerivedFormat::LOWER};
constexpr DerivedAndBaseFormat HEX_UPPER{{BaseFormat::HEX}, DerivedAndBaseFormat::DerivedFormat::UPPER};

BOOST_AUTO_TEST_CASE(with_params_derived)
{
    Derived d;
    d.m_base_data = 0x0F;
    d.m_derived_data = "xY";

    DataStream stream;

    stream << RAW_LOWER(d);

    stream << HEX_UPPER(d);

    BOOST_CHECK_EQUAL(stream.str(), "\x0F\x02xy"
                                    "0f\x02XY");
}

BOOST_AUTO_TEST_SUITE_END()
