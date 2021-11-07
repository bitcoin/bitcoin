// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SERIALIZE_H
#define BITCOIN_SERIALIZE_H

#include <compat/endian.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <ios>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string.h>
#include <utility>
#include <vector>

#include <prevector.h>
#include <span.h>

/**
 * The maximum size of a serialized object in bytes or number of elements
 * (for eg vectors) when the size is encoded as CompactSize.
 */
static constexpr uint64_t MAX_SIZE = 0x02000000;

/** Maximum amount of memory (in bytes) to allocate at once when deserializing vectors. */
static const unsigned int MAX_VECTOR_ALLOCATE = 5000000;

/**
 * Dummy data type to identify deserializing constructors.
 *
 * By convention, a constructor of a type T with signature
 *
 *   template <typename Stream> T::T(deserialize_type, Stream& s)
 *
 * is a deserializing constructor, which builds the type by
 * deserializing it from s. If T contains const fields, this
 * is likely the only way to do so.
 */
struct deserialize_type {};
constexpr deserialize_type deserialize {};

//! Safely convert odd char pointer types to standard ones.
inline char* CharCast(char* c) { return c; }
inline char* CharCast(unsigned char* c) { return (char*)c; }
inline const char* CharCast(const char* c) { return c; }
inline const char* CharCast(const unsigned char* c) { return (const char*)c; }

/*
 * Lowest-level serialization and conversion.
 * @note Sizes of these types are verified in the tests
 */
template<typename Stream> inline void ser_writedata8(Stream &s, uint8_t obj)
{
    s.write((char*)&obj, 1);
}
template<typename Stream> inline void ser_writedata16(Stream &s, uint16_t obj)
{
    obj = htole16(obj);
    s.write((char*)&obj, 2);
}
template<typename Stream> inline void ser_writedata16be(Stream &s, uint16_t obj)
{
    obj = htobe16(obj);
    s.write((char*)&obj, 2);
}
template<typename Stream> inline void ser_writedata32(Stream &s, uint32_t obj)
{
    obj = htole32(obj);
    s.write((char*)&obj, 4);
}
template<typename Stream> inline void ser_writedata32be(Stream &s, uint32_t obj)
{
    obj = htobe32(obj);
    s.write((char*)&obj, 4);
}
template<typename Stream> inline void ser_writedata64(Stream &s, uint64_t obj)
{
    obj = htole64(obj);
    s.write((char*)&obj, 8);
}
template<typename Stream> inline uint8_t ser_readdata8(Stream &s)
{
    uint8_t obj;
    s.read((char*)&obj, 1);
    return obj;
}
template<typename Stream> inline uint16_t ser_readdata16(Stream &s)
{
    uint16_t obj;
    s.read((char*)&obj, 2);
    return le16toh(obj);
}
template<typename Stream> inline uint16_t ser_readdata16be(Stream &s)
{
    uint16_t obj;
    s.read((char*)&obj, 2);
    return be16toh(obj);
}
template<typename Stream> inline uint32_t ser_readdata32(Stream &s)
{
    uint32_t obj;
    s.read((char*)&obj, 4);
    return le32toh(obj);
}
template<typename Stream> inline uint32_t ser_readdata32be(Stream &s)
{
    uint32_t obj;
    s.read((char*)&obj, 4);
    return be32toh(obj);
}
template<typename Stream> inline uint64_t ser_readdata64(Stream &s)
{
    uint64_t obj;
    s.read((char*)&obj, 8);
    return le64toh(obj);
}


/////////////////////////////////////////////////////////////////
//
// Templates for serializing to anything that looks like a stream,
// i.e. anything that supports .read(char*, size_t) and .write(char*, size_t)
//

class CSizeComputer;

enum
{
    // primary actions
    SER_NETWORK         = (1 << 0),
    SER_DISK            = (1 << 1),
    SER_GETHASH         = (1 << 2),
};

//! Convert the reference base type to X, without changing constness or reference type.
template<typename X> X& ReadWriteAsHelper(X& x) { return x; }
template<typename X> const X& ReadWriteAsHelper(const X& x) { return x; }

#define READWRITE(...) (::SerReadWriteMany(s, ser_action, __VA_ARGS__))
#define READWRITEAS(type, obj) (::SerReadWriteMany(s, ser_action, ReadWriteAsHelper<type>(obj)))
#define SER_READ(obj, code) ::SerRead(s, ser_action, obj, [&](Stream& s, typename std::remove_const<Type>::type& obj) { code; })
#define SER_WRITE(obj, code) ::SerWrite(s, ser_action, obj, [&](Stream& s, const Type& obj) { code; })

/**
 * Implement the Ser and Unser methods needed for implementing a formatter (see Using below).
 *
 * Both Ser and Unser are delegated to a single static method SerializationOps, which is polymorphic
 * in the serialized/deserialized type (allowing it to be const when serializing, and non-const when
 * deserializing).
 *
 * Example use:
 *   struct FooFormatter {
 *     FORMATTER_METHODS(Class, obj) { READWRITE(obj.val1, VARINT(obj.val2)); }
 *   }
 *   would define a class FooFormatter that defines a serialization of Class objects consisting
 *   of serializing its val1 member using the default serialization, and its val2 member using
 *   VARINT serialization. That FooFormatter can then be used in statements like
 *   READWRITE(Using<FooFormatter>(obj.bla)).
 */
#define FORMATTER_METHODS(cls, obj) \
    template<typename Stream> \
    static void Ser(Stream& s, const cls& obj) { SerializationOps(obj, s, CSerActionSerialize()); } \
    template<typename Stream> \
    static void Unser(Stream& s, cls& obj) { SerializationOps(obj, s, CSerActionUnserialize()); } \
    template<typename Stream, typename Type, typename Operation> \
    static inline void SerializationOps(Type& obj, Stream& s, Operation ser_action) \

/**
 * Implement the Serialize and Unserialize methods by delegating to a single templated
 * static method that takes the to-be-(de)serialized object as a parameter. This approach
 * has the advantage that the constness of the object becomes a template parameter, and
 * thus allows a single implementation that sees the object as const for serializing
 * and non-const for deserializing, without casts.
 */
#define SERIALIZE_METHODS(cls, obj)                                                 \
    template<typename Stream>                                                       \
    void Serialize(Stream& s) const                                                 \
    {                                                                               \
        static_assert(std::is_same<const cls&, decltype(*this)>::value, "Serialize type mismatch"); \
        Ser(s, *this);                                                              \
    }                                                                               \
    template<typename Stream>                                                       \
    void Unserialize(Stream& s)                                                     \
    {                                                                               \
        static_assert(std::is_same<cls&, decltype(*this)>::value, "Unserialize type mismatch"); \
        Unser(s, *this);                                                            \
    }                                                                               \
    FORMATTER_METHODS(cls, obj)

#ifndef CHAR_EQUALS_INT8
template<typename Stream> inline void Serialize(Stream& s, char a    ) { ser_writedata8(s, a); } // TODO Get rid of bare char
#endif
template<typename Stream> inline void Serialize(Stream& s, int8_t a  ) { ser_writedata8(s, a); }
template<typename Stream> inline void Serialize(Stream& s, uint8_t a ) { ser_writedata8(s, a); }
template<typename Stream> inline void Serialize(Stream& s, int16_t a ) { ser_writedata16(s, a); }
template<typename Stream> inline void Serialize(Stream& s, uint16_t a) { ser_writedata16(s, a); }
template<typename Stream> inline void Serialize(Stream& s, int32_t a ) { ser_writedata32(s, a); }
template<typename Stream> inline void Serialize(Stream& s, uint32_t a) { ser_writedata32(s, a); }
template<typename Stream> inline void Serialize(Stream& s, int64_t a ) { ser_writedata64(s, a); }
template<typename Stream> inline void Serialize(Stream& s, uint64_t a) { ser_writedata64(s, a); }
template<typename Stream, int N> inline void Serialize(Stream& s, const char (&a)[N]) { s.write(a, N); }
template<typename Stream, int N> inline void Serialize(Stream& s, const unsigned char (&a)[N]) { s.write(CharCast(a), N); }
template<typename Stream> inline void Serialize(Stream& s, const Span<const unsigned char>& span) { s.write(CharCast(span.data()), span.size()); }
template<typename Stream> inline void Serialize(Stream& s, const Span<unsigned char>& span) { s.write(CharCast(span.data()), span.size()); }

#ifndef CHAR_EQUALS_INT8
template<typename Stream> inline void Unserialize(Stream& s, char& a    ) { a = ser_readdata8(s); } // TODO Get rid of bare char
#endif
template<typename Stream> inline void Unserialize(Stream& s, int8_t& a  ) { a = ser_readdata8(s); }
template<typename Stream> inline void Unserialize(Stream& s, uint8_t& a ) { a = ser_readdata8(s); }
template<typename Stream> inline void Unserialize(Stream& s, int16_t& a ) { a = ser_readdata16(s); }
template<typename Stream> inline void Unserialize(Stream& s, uint16_t& a) { a = ser_readdata16(s); }
template<typename Stream> inline void Unserialize(Stream& s, int32_t& a ) { a = ser_readdata32(s); }
template<typename Stream> inline void Unserialize(Stream& s, uint32_t& a) { a = ser_readdata32(s); }
template<typename Stream> inline void Unserialize(Stream& s, int64_t& a ) { a = ser_readdata64(s); }
template<typename Stream> inline void Unserialize(Stream& s, uint64_t& a) { a = ser_readdata64(s); }
template<typename Stream, int N> inline void Unserialize(Stream& s, char (&a)[N]) { s.read(a, N); }
template<typename Stream, int N> inline void Unserialize(Stream& s, unsigned char (&a)[N]) { s.read(CharCast(a), N); }
template<typename Stream> inline void Unserialize(Stream& s, Span<unsigned char>& span) { s.read(CharCast(span.data()), span.size()); }

template <typename Stream> inline void Serialize(Stream& s, bool a) { uint8_t f = a; ser_writedata8(s, f); }
template <typename Stream> inline void Unserialize(Stream& s, bool& a) { uint8_t f = ser_readdata8(s); a = f; }


/**
 * Compact Size
 * size <  253        -- 1 byte
 * size <= USHRT_MAX  -- 3 bytes  (253 + 2 bytes)
 * size <= UINT_MAX   -- 5 bytes  (254 + 4 bytes)
 * size >  UINT_MAX   -- 9 bytes  (255 + 8 bytes)
 */
inline unsigned int GetSizeOfCompactSize(uint64_t nSize)
{
    if (nSize < 253)             return sizeof(unsigned char);
    else if (nSize <= std::numeric_limits<uint16_t>::max()) return sizeof(unsigned char) + sizeof(uint16_t);
    else if (nSize <= std::numeric_limits<unsigned int>::max())  return sizeof(unsigned char) + sizeof(unsigned int);
    else                         return sizeof(unsigned char) + sizeof(uint64_t);
}

inline void WriteCompactSize(CSizeComputer& os, uint64_t nSize);

template<typename Stream>
void WriteCompactSize(Stream& os, uint64_t nSize)
{
    if (nSize < 253)
    {
        ser_writedata8(os, nSize);
    }
    else if (nSize <= std::numeric_limits<uint16_t>::max())
    {
        ser_writedata8(os, 253);
        ser_writedata16(os, nSize);
    }
    else if (nSize <= std::numeric_limits<unsigned int>::max())
    {
        ser_writedata8(os, 254);
        ser_writedata32(os, nSize);
    }
    else
    {
        ser_writedata8(os, 255);
        ser_writedata64(os, nSize);
    }
    return;
}

/**
 * Decode a CompactSize-encoded variable-length integer.
 *
 * As these are primarily used to encode the size of vector-like serializations, by default a range
 * check is performed. When used as a generic number encoding, range_check should be set to false.
 */
template<typename Stream>
uint64_t ReadCompactSize(Stream& is, bool range_check = true)
{
    uint8_t chSize = ser_readdata8(is);
    uint64_t nSizeRet = 0;
    if (chSize < 253)
    {
        nSizeRet = chSize;
    }
    else if (chSize == 253)
    {
        nSizeRet = ser_readdata16(is);
        if (nSizeRet < 253)
            throw std::ios_base::failure("non-canonical ReadCompactSize()");
    }
    else if (chSize == 254)
    {
        nSizeRet = ser_readdata32(is);
        if (nSizeRet < 0x10000u)
            throw std::ios_base::failure("non-canonical ReadCompactSize()");
    }
    else
    {
        nSizeRet = ser_readdata64(is);
        if (nSizeRet < 0x100000000ULL)
            throw std::ios_base::failure("non-canonical ReadCompactSize()");
    }
    if (range_check && nSizeRet > MAX_SIZE) {
        throw std::ios_base::failure("ReadCompactSize(): size too large");
    }
    return nSizeRet;
}

/**
 * Variable-length integers: bytes are a MSB base-128 encoding of the number.
 * The high bit in each byte signifies whether another digit follows. To make
 * sure the encoding is one-to-one, one is subtracted from all but the last digit.
 * Thus, the byte sequence a[] with length len, where all but the last byte
 * has bit 128 set, encodes the number:
 *
 *  (a[len-1] & 0x7F) + sum(i=1..len-1, 128^i*((a[len-i-1] & 0x7F)+1))
 *
 * Properties:
 * * Very small (0-127: 1 byte, 128-16511: 2 bytes, 16512-2113663: 3 bytes)
 * * Every integer has exactly one encoding
 * * Encoding does not depend on size of original integer type
 * * No redundancy: every (infinite) byte sequence corresponds to a list
 *   of encoded integers.
 *
 * 0:         [0x00]  256:        [0x81 0x00]
 * 1:         [0x01]  16383:      [0xFE 0x7F]
 * 127:       [0x7F]  16384:      [0xFF 0x00]
 * 128:  [0x80 0x00]  16511:      [0xFF 0x7F]
 * 255:  [0x80 0x7F]  65535: [0x82 0xFE 0x7F]
 * 2^32:           [0x8E 0xFE 0xFE 0xFF 0x00]
 */

/**
 * Mode for encoding VarInts.
 *
 * Currently there is no support for signed encodings. The default mode will not
 * compile with signed values, and the legacy "nonnegative signed" mode will
 * accept signed values, but improperly encode and decode them if they are
 * negative. In the future, the DEFAULT mode could be extended to support
 * negative numbers in a backwards compatible way, and additional modes could be
 * added to support different varint formats (e.g. zigzag encoding).
 */
enum class VarIntMode { DEFAULT, NONNEGATIVE_SIGNED };

template <VarIntMode Mode, typename I>
struct CheckVarIntMode {
    constexpr CheckVarIntMode()
    {
        static_assert(Mode != VarIntMode::DEFAULT || std::is_unsigned<I>::value, "Unsigned type required with mode DEFAULT.");
        static_assert(Mode != VarIntMode::NONNEGATIVE_SIGNED || std::is_signed<I>::value, "Signed type required with mode NONNEGATIVE_SIGNED.");
    }
};

template<VarIntMode Mode, typename I>
inline unsigned int GetSizeOfVarInt(I n)
{
    CheckVarIntMode<Mode, I>();
    int nRet = 0;
    while(true) {
        nRet++;
        if (n <= 0x7F)
            break;
        n = (n >> 7) - 1;
    }
    return nRet;
}

template<typename I>
inline void WriteVarInt(CSizeComputer& os, I n);

template<typename Stream, VarIntMode Mode, typename I>
void WriteVarInt(Stream& os, I n)
{
    CheckVarIntMode<Mode, I>();
    unsigned char tmp[(sizeof(n)*8+6)/7];
    int len=0;
    while(true) {
        tmp[len] = (n & 0x7F) | (len ? 0x80 : 0x00);
        if (n <= 0x7F)
            break;
        n = (n >> 7) - 1;
        len++;
    }
    do {
        ser_writedata8(os, tmp[len]);
    } while(len--);
}

template<typename Stream, VarIntMode Mode, typename I>
I ReadVarInt(Stream& is)
{
    CheckVarIntMode<Mode, I>();
    I n = 0;
    while(true) {
        unsigned char chData = ser_readdata8(is);
        if (n > (std::numeric_limits<I>::max() >> 7)) {
           throw std::ios_base::failure("ReadVarInt(): size too large");
        }
        n = (n << 7) | (chData & 0x7F);
        if (chData & 0x80) {
            if (n == std::numeric_limits<I>::max()) {
                throw std::ios_base::failure("ReadVarInt(): size too large");
            }
            n++;
        } else {
            return n;
        }
    }
}

/** Simple wrapper class to serialize objects using a formatter; used by Using(). */
template<typename Formatter, typename T>
class Wrapper
{
    static_assert(std::is_lvalue_reference<T>::value, "Wrapper needs an lvalue reference type T");
protected:
    T m_object;
public:
    explicit Wrapper(T obj) : m_object(obj) {}
    template<typename Stream> void Serialize(Stream &s) const { Formatter().Ser(s, m_object); }
    template<typename Stream> void Unserialize(Stream &s) { Formatter().Unser(s, m_object); }
};

/** Cause serialization/deserialization of an object to be done using a specified formatter class.
 *
 * To use this, you need a class Formatter that has public functions Ser(stream, const object&) for
 * serialization, and Unser(stream, object&) for deserialization. Serialization routines (inside
 * READWRITE, or directly with << and >> operators), can then use Using<Formatter>(object).
 *
 * This works by constructing a Wrapper<Formatter, T>-wrapped version of object, where T is
 * const during serialization, and non-const during deserialization, which maintains const
 * correctness.
 */
template<typename Formatter, typename T>
static inline Wrapper<Formatter, T&> Using(T&& t) { return Wrapper<Formatter, T&>(t); }

#define VARINT_MODE(obj, mode) Using<VarIntFormatter<mode>>(obj)
#define VARINT(obj) Using<VarIntFormatter<VarIntMode::DEFAULT>>(obj)
#define COMPACTSIZE(obj) Using<CompactSizeFormatter<true>>(obj)
#define LIMITED_STRING(obj,n) Using<LimitedStringFormatter<n>>(obj)

/** Serialization wrapper class for integers in VarInt format. */
template<VarIntMode Mode>
struct VarIntFormatter
{
    template<typename Stream, typename I> void Ser(Stream &s, I v)
    {
        WriteVarInt<Stream,Mode,typename std::remove_cv<I>::type>(s, v);
    }

    template<typename Stream, typename I> void Unser(Stream& s, I& v)
    {
        v = ReadVarInt<Stream,Mode,typename std::remove_cv<I>::type>(s);
    }
};

/** Serialization wrapper class for custom integers and enums.
 *
 * It permits specifying the serialized size (1 to 8 bytes) and endianness.
 *
 * Use the big endian mode for values that are stored in memory in native
 * byte order, but serialized in big endian notation. This is only intended
 * to implement serializers that are compatible with existing formats, and
 * its use is not recommended for new data structures.
 */
template<int Bytes, bool BigEndian = false>
struct CustomUintFormatter
{
    static_assert(Bytes > 0 && Bytes <= 8, "CustomUintFormatter Bytes out of range");
    static constexpr uint64_t MAX = 0xffffffffffffffff >> (8 * (8 - Bytes));

    template <typename Stream, typename I> void Ser(Stream& s, I v)
    {
        if (v < 0 || v > MAX) throw std::ios_base::failure("CustomUintFormatter value out of range");
        if (BigEndian) {
            uint64_t raw = htobe64(v);
            s.write(((const char*)&raw) + 8 - Bytes, Bytes);
        } else {
            uint64_t raw = htole64(v);
            s.write((const char*)&raw, Bytes);
        }
    }

    template <typename Stream, typename I> void Unser(Stream& s, I& v)
    {
        using U = typename std::conditional<std::is_enum<I>::value, std::underlying_type<I>, std::common_type<I>>::type::type;
        static_assert(std::numeric_limits<U>::max() >= MAX && std::numeric_limits<U>::min() <= 0, "Assigned type too small");
        uint64_t raw = 0;
        if (BigEndian) {
            s.read(((char*)&raw) + 8 - Bytes, Bytes);
            v = static_cast<I>(be64toh(raw));
        } else {
            s.read((char*)&raw, Bytes);
            v = static_cast<I>(le64toh(raw));
        }
    }
};

template<int Bytes> using BigEndianFormatter = CustomUintFormatter<Bytes, true>;

/** Formatter for integers in CompactSize format. */
template<bool RangeCheck>
struct CompactSizeFormatter
{
    template<typename Stream, typename I>
    void Unser(Stream& s, I& v)
    {
        uint64_t n = ReadCompactSize<Stream>(s, RangeCheck);
        if (n < std::numeric_limits<I>::min() || n > std::numeric_limits<I>::max()) {
            throw std::ios_base::failure("CompactSize exceeds limit of type");
        }
        v = n;
    }

    template<typename Stream, typename I>
    void Ser(Stream& s, I v)
    {
        static_assert(std::is_unsigned<I>::value, "CompactSize only supported for unsigned integers");
        static_assert(std::numeric_limits<I>::max() <= std::numeric_limits<uint64_t>::max(), "CompactSize only supports 64-bit integers and below");

        WriteCompactSize<Stream>(s, v);
    }
};

template<size_t Limit>
struct LimitedStringFormatter
{
    template<typename Stream>
    void Unser(Stream& s, std::string& v)
    {
        size_t size = ReadCompactSize(s);
        if (size > Limit) {
            throw std::ios_base::failure("String length limit exceeded");
        }
        v.resize(size);
        if (size != 0) s.read((char*)v.data(), size);
    }

    template<typename Stream>
    void Ser(Stream& s, const std::string& v)
    {
        s << v;
    }
};

/** Formatter to serialize/deserialize vector elements using another formatter
 *
 * Example:
 *   struct X {
 *     std::vector<uint64_t> v;
 *     SERIALIZE_METHODS(X, obj) { READWRITE(Using<VectorFormatter<VarInt>>(obj.v)); }
 *   };
 * will define a struct that contains a vector of uint64_t, which is serialized
 * as a vector of VarInt-encoded integers.
 *
 * V is not required to be an std::vector type. It works for any class that
 * exposes a value_type, size, reserve, emplace_back, back, and const iterators.
 */
template<class Formatter>
struct VectorFormatter
{
    template<typename Stream, typename V>
    void Ser(Stream& s, const V& v)
    {
        Formatter formatter;
        WriteCompactSize(s, v.size());
        for (const typename V::value_type& elem : v) {
            formatter.Ser(s, elem);
        }
    }

    template<typename Stream, typename V>
    void Unser(Stream& s, V& v)
    {
        Formatter formatter;
        v.clear();
        size_t size = ReadCompactSize(s);
        size_t allocated = 0;
        while (allocated < size) {
            // For DoS prevention, do not blindly allocate as much as the stream claims to contain.
            // Instead, allocate in 5MiB batches, so that an attacker actually needs to provide
            // X MiB of data to make us allocate X+5 Mib.
            static_assert(sizeof(typename V::value_type) <= MAX_VECTOR_ALLOCATE, "Vector element size too large");
            allocated = std::min(size, allocated + MAX_VECTOR_ALLOCATE / sizeof(typename V::value_type));
            v.reserve(allocated);
            while (v.size() < allocated) {
                v.emplace_back();
                formatter.Unser(s, v.back());
            }
        }
    };
};

/**
 * Forward declarations
 */

/**
 *  string
 */
template<typename Stream, typename C> void Serialize(Stream& os, const std::basic_string<C>& str);
template<typename Stream, typename C> void Unserialize(Stream& is, std::basic_string<C>& str);

/**
 * prevector
 * prevectors of unsigned char are a special case and are intended to be serialized as a single opaque blob.
 */
template<typename Stream, unsigned int N, typename T> void Serialize_impl(Stream& os, const prevector<N, T>& v, const unsigned char&);
template<typename Stream, unsigned int N, typename T, typename V> void Serialize_impl(Stream& os, const prevector<N, T>& v, const V&);
template<typename Stream, unsigned int N, typename T> inline void Serialize(Stream& os, const prevector<N, T>& v);
template<typename Stream, unsigned int N, typename T> void Unserialize_impl(Stream& is, prevector<N, T>& v, const unsigned char&);
template<typename Stream, unsigned int N, typename T, typename V> void Unserialize_impl(Stream& is, prevector<N, T>& v, const V&);
template<typename Stream, unsigned int N, typename T> inline void Unserialize(Stream& is, prevector<N, T>& v);

/**
 * vector
 * vectors of unsigned char are a special case and are intended to be serialized as a single opaque blob.
 */
template<typename Stream, typename T, typename A> void Serialize_impl(Stream& os, const std::vector<T, A>& v, const unsigned char&);
template<typename Stream, typename T, typename A> void Serialize_impl(Stream& os, const std::vector<T, A>& v, const bool&);
template<typename Stream, typename T, typename A, typename V> void Serialize_impl(Stream& os, const std::vector<T, A>& v, const V&);
template<typename Stream, typename T, typename A> inline void Serialize(Stream& os, const std::vector<T, A>& v);
template<typename Stream, typename T, typename A> void Unserialize_impl(Stream& is, std::vector<T, A>& v, const unsigned char&);
template<typename Stream, typename T, typename A, typename V> void Unserialize_impl(Stream& is, std::vector<T, A>& v, const V&);
template<typename Stream, typename T, typename A> inline void Unserialize(Stream& is, std::vector<T, A>& v);

/**
 * pair
 */
template<typename Stream, typename K, typename T> void Serialize(Stream& os, const std::pair<K, T>& item);
template<typename Stream, typename K, typename T> void Unserialize(Stream& is, std::pair<K, T>& item);

/**
 * map
 */
template<typename Stream, typename K, typename T, typename Pred, typename A> void Serialize(Stream& os, const std::map<K, T, Pred, A>& m);
template<typename Stream, typename K, typename T, typename Pred, typename A> void Unserialize(Stream& is, std::map<K, T, Pred, A>& m);

/**
 * set
 */
template<typename Stream, typename K, typename Pred, typename A> void Serialize(Stream& os, const std::set<K, Pred, A>& m);
template<typename Stream, typename K, typename Pred, typename A> void Unserialize(Stream& is, std::set<K, Pred, A>& m);

/**
 * shared_ptr
 */
template<typename Stream, typename T> void Serialize(Stream& os, const std::shared_ptr<const T>& p);
template<typename Stream, typename T> void Unserialize(Stream& os, std::shared_ptr<const T>& p);

/**
 * unique_ptr
 */
template<typename Stream, typename T> void Serialize(Stream& os, const std::unique_ptr<const T>& p);
template<typename Stream, typename T> void Unserialize(Stream& os, std::unique_ptr<const T>& p);



/**
 * If none of the specialized versions above matched, default to calling member function.
 */
template<typename Stream, typename T>
inline void Serialize(Stream& os, const T& a)
{
    a.Serialize(os);
}

template<typename Stream, typename T>
inline void Unserialize(Stream& is, T&& a)
{
    a.Unserialize(is);
}

/** Default formatter. Serializes objects as themselves.
 *
 * The vector/prevector serialization code passes this to VectorFormatter
 * to enable reusing that logic. It shouldn't be needed elsewhere.
 */
struct DefaultFormatter
{
    template<typename Stream, typename T>
    static void Ser(Stream& s, const T& t) { Serialize(s, t); }

    template<typename Stream, typename T>
    static void Unser(Stream& s, T& t) { Unserialize(s, t); }
};





/**
 * string
 */
template<typename Stream, typename C>
void Serialize(Stream& os, const std::basic_string<C>& str)
{
    WriteCompactSize(os, str.size());
    if (!str.empty())
        os.write((char*)str.data(), str.size() * sizeof(C));
}

template<typename Stream, typename C>
void Unserialize(Stream& is, std::basic_string<C>& str)
{
    unsigned int nSize = ReadCompactSize(is);
    str.resize(nSize);
    if (nSize != 0)
        is.read((char*)str.data(), nSize * sizeof(C));
}



/**
 * prevector
 */
template<typename Stream, unsigned int N, typename T>
void Serialize_impl(Stream& os, const prevector<N, T>& v, const unsigned char&)
{
    WriteCompactSize(os, v.size());
    if (!v.empty())
        os.write((char*)v.data(), v.size() * sizeof(T));
}

template<typename Stream, unsigned int N, typename T, typename V>
void Serialize_impl(Stream& os, const prevector<N, T>& v, const V&)
{
    Serialize(os, Using<VectorFormatter<DefaultFormatter>>(v));
}

template<typename Stream, unsigned int N, typename T>
inline void Serialize(Stream& os, const prevector<N, T>& v)
{
    Serialize_impl(os, v, T());
}


template<typename Stream, unsigned int N, typename T>
void Unserialize_impl(Stream& is, prevector<N, T>& v, const unsigned char&)
{
    // Limit size per read so bogus size value won't cause out of memory
    v.clear();
    unsigned int nSize = ReadCompactSize(is);
    unsigned int i = 0;
    while (i < nSize)
    {
        unsigned int blk = std::min(nSize - i, (unsigned int)(1 + 4999999 / sizeof(T)));
        v.resize_uninitialized(i + blk);
        is.read((char*)&v[i], blk * sizeof(T));
        i += blk;
    }
}

template<typename Stream, unsigned int N, typename T, typename V>
void Unserialize_impl(Stream& is, prevector<N, T>& v, const V&)
{
    Unserialize(is, Using<VectorFormatter<DefaultFormatter>>(v));
}

template<typename Stream, unsigned int N, typename T>
inline void Unserialize(Stream& is, prevector<N, T>& v)
{
    Unserialize_impl(is, v, T());
}



/**
 * vector
 */
template<typename Stream, typename T, typename A>
void Serialize_impl(Stream& os, const std::vector<T, A>& v, const unsigned char&)
{
    WriteCompactSize(os, v.size());
    if (!v.empty())
        os.write((char*)v.data(), v.size() * sizeof(T));
}

template<typename Stream, typename T, typename A>
void Serialize_impl(Stream& os, const std::vector<T, A>& v, const bool&)
{
    // A special case for std::vector<bool>, as dereferencing
    // std::vector<bool>::const_iterator does not result in a const bool&
    // due to std::vector's special casing for bool arguments.
    WriteCompactSize(os, v.size());
    for (bool elem : v) {
        ::Serialize(os, elem);
    }
}

template<typename Stream, typename T, typename A, typename V>
void Serialize_impl(Stream& os, const std::vector<T, A>& v, const V&)
{
    Serialize(os, Using<VectorFormatter<DefaultFormatter>>(v));
}

template<typename Stream, typename T, typename A>
inline void Serialize(Stream& os, const std::vector<T, A>& v)
{
    Serialize_impl(os, v, T());
}


template<typename Stream, typename T, typename A>
void Unserialize_impl(Stream& is, std::vector<T, A>& v, const unsigned char&)
{
    // Limit size per read so bogus size value won't cause out of memory
    v.clear();
    unsigned int nSize = ReadCompactSize(is);
    unsigned int i = 0;
    while (i < nSize)
    {
        unsigned int blk = std::min(nSize - i, (unsigned int)(1 + 4999999 / sizeof(T)));
        v.resize(i + blk);
        is.read((char*)&v[i], blk * sizeof(T));
        i += blk;
    }
}

template<typename Stream, typename T, typename A, typename V>
void Unserialize_impl(Stream& is, std::vector<T, A>& v, const V&)
{
    Unserialize(is, Using<VectorFormatter<DefaultFormatter>>(v));
}

template<typename Stream, typename T, typename A>
inline void Unserialize(Stream& is, std::vector<T, A>& v)
{
    Unserialize_impl(is, v, T());
}



/**
 * pair
 */
template<typename Stream, typename K, typename T>
void Serialize(Stream& os, const std::pair<K, T>& item)
{
    Serialize(os, item.first);
    Serialize(os, item.second);
}

template<typename Stream, typename K, typename T>
void Unserialize(Stream& is, std::pair<K, T>& item)
{
    Unserialize(is, item.first);
    Unserialize(is, item.second);
}



/**
 * map
 */
template<typename Stream, typename K, typename T, typename Pred, typename A>
void Serialize(Stream& os, const std::map<K, T, Pred, A>& m)
{
    WriteCompactSize(os, m.size());
    for (const auto& entry : m)
        Serialize(os, entry);
}

template<typename Stream, typename K, typename T, typename Pred, typename A>
void Unserialize(Stream& is, std::map<K, T, Pred, A>& m)
{
    m.clear();
    unsigned int nSize = ReadCompactSize(is);
    typename std::map<K, T, Pred, A>::iterator mi = m.begin();
    for (unsigned int i = 0; i < nSize; i++)
    {
        std::pair<K, T> item;
        Unserialize(is, item);
        mi = m.insert(mi, item);
    }
}



/**
 * set
 */
template<typename Stream, typename K, typename Pred, typename A>
void Serialize(Stream& os, const std::set<K, Pred, A>& m)
{
    WriteCompactSize(os, m.size());
    for (typename std::set<K, Pred, A>::const_iterator it = m.begin(); it != m.end(); ++it)
        Serialize(os, (*it));
}

template<typename Stream, typename K, typename Pred, typename A>
void Unserialize(Stream& is, std::set<K, Pred, A>& m)
{
    m.clear();
    unsigned int nSize = ReadCompactSize(is);
    typename std::set<K, Pred, A>::iterator it = m.begin();
    for (unsigned int i = 0; i < nSize; i++)
    {
        K key;
        Unserialize(is, key);
        it = m.insert(it, key);
    }
}



/**
 * unique_ptr
 */
template<typename Stream, typename T> void
Serialize(Stream& os, const std::unique_ptr<const T>& p)
{
    Serialize(os, *p);
}

template<typename Stream, typename T>
void Unserialize(Stream& is, std::unique_ptr<const T>& p)
{
    p.reset(new T(deserialize, is));
}



/**
 * shared_ptr
 */
template<typename Stream, typename T> void
Serialize(Stream& os, const std::shared_ptr<const T>& p)
{
    Serialize(os, *p);
}

template<typename Stream, typename T>
void Unserialize(Stream& is, std::shared_ptr<const T>& p)
{
    p = std::make_shared<const T>(deserialize, is);
}



/**
 * Support for SERIALIZE_METHODS and READWRITE macro.
 */
struct CSerActionSerialize
{
    constexpr bool ForRead() const { return false; }
};
struct CSerActionUnserialize
{
    constexpr bool ForRead() const { return true; }
};








/* ::GetSerializeSize implementations
 *
 * Computing the serialized size of objects is done through a special stream
 * object of type CSizeComputer, which only records the number of bytes written
 * to it.
 *
 * If your Serialize or SerializationOp method has non-trivial overhead for
 * serialization, it may be worthwhile to implement a specialized version for
 * CSizeComputer, which uses the s.seek() method to record bytes that would
 * be written instead.
 */
class CSizeComputer
{
protected:
    size_t nSize;

    const int nVersion;
public:
    explicit CSizeComputer(int nVersionIn) : nSize(0), nVersion(nVersionIn) {}

    void write(const char *psz, size_t _nSize)
    {
        this->nSize += _nSize;
    }

    /** Pretend _nSize bytes are written, without specifying them. */
    void seek(size_t _nSize)
    {
        this->nSize += _nSize;
    }

    template<typename T>
    CSizeComputer& operator<<(const T& obj)
    {
        ::Serialize(*this, obj);
        return (*this);
    }

    size_t size() const {
        return nSize;
    }

    int GetVersion() const { return nVersion; }
};

template<typename Stream>
void SerializeMany(Stream& s)
{
}

template<typename Stream, typename Arg, typename... Args>
void SerializeMany(Stream& s, const Arg& arg, const Args&... args)
{
    ::Serialize(s, arg);
    ::SerializeMany(s, args...);
}

template<typename Stream>
inline void UnserializeMany(Stream& s)
{
}

template<typename Stream, typename Arg, typename... Args>
inline void UnserializeMany(Stream& s, Arg&& arg, Args&&... args)
{
    ::Unserialize(s, arg);
    ::UnserializeMany(s, args...);
}

template<typename Stream, typename... Args>
inline void SerReadWriteMany(Stream& s, CSerActionSerialize ser_action, const Args&... args)
{
    ::SerializeMany(s, args...);
}

template<typename Stream, typename... Args>
inline void SerReadWriteMany(Stream& s, CSerActionUnserialize ser_action, Args&&... args)
{
    ::UnserializeMany(s, args...);
}

template<typename Stream, typename Type, typename Fn>
inline void SerRead(Stream& s, CSerActionSerialize ser_action, Type&&, Fn&&)
{
}

template<typename Stream, typename Type, typename Fn>
inline void SerRead(Stream& s, CSerActionUnserialize ser_action, Type&& obj, Fn&& fn)
{
    fn(s, std::forward<Type>(obj));
}

template<typename Stream, typename Type, typename Fn>
inline void SerWrite(Stream& s, CSerActionSerialize ser_action, Type&& obj, Fn&& fn)
{
    fn(s, std::forward<Type>(obj));
}

template<typename Stream, typename Type, typename Fn>
inline void SerWrite(Stream& s, CSerActionUnserialize ser_action, Type&&, Fn&&)
{
}

template<typename I>
inline void WriteVarInt(CSizeComputer &s, I n)
{
    s.seek(GetSizeOfVarInt<I>(n));
}

inline void WriteCompactSize(CSizeComputer &s, uint64_t nSize)
{
    s.seek(GetSizeOfCompactSize(nSize));
}

template <typename T>
size_t GetSerializeSize(const T& t, int nVersion = 0)
{
    return (CSizeComputer(nVersion) << t).size();
}

template <typename... T>
size_t GetSerializeSizeMany(int nVersion, const T&... t)
{
    CSizeComputer sc(nVersion);
    SerializeMany(sc, t...);
    return sc.size();
}

#endif // BITCOIN_SERIALIZE_H
