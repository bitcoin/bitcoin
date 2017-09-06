// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SERIALIZE_H
#define BITCOIN_SERIALIZE_H

#include <compat/endian.h>

#include <algorithm>
#include <assert.h>
#include <ios>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <stdint.h>
#include <string>
#include <string.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <prevector.h>
#include <span.h>

static const unsigned int MAX_SIZE = 0x02000000;

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

/**
 * Used to bypass the rule against non-const reference to temporary
 * where it makes sense with wrappers.
 */
template<typename T>
inline T& REF(const T& val)
{
    return const_cast<T&>(val);
}

/**
 * Used to acquire a non-const pointer "this" to generate bodies
 * of const serialization operations from a template
 */
template<typename T>
inline T* NCONST_PTR(const T* val)
{
    return const_cast<T*>(val);
}

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
inline uint64_t ser_double_to_uint64(double x)
{
    union { double x; uint64_t y; } tmp;
    tmp.x = x;
    return tmp.y;
}
inline uint32_t ser_float_to_uint32(float x)
{
    union { float x; uint32_t y; } tmp;
    tmp.x = x;
    return tmp.y;
}
inline double ser_uint64_to_double(uint64_t y)
{
    union { double x; uint64_t y; } tmp;
    tmp.y = y;
    return tmp.x;
}
inline float ser_uint32_to_float(uint32_t y)
{
    union { float x; uint32_t y; } tmp;
    tmp.y = y;
    return tmp.x;
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

/**
 * Implement three methods for serializable objects. These are actually wrappers over
 * "SerializationOp" template, which implements the body of each class' serialization
 * code. Adding "ADD_SERIALIZE_METHODS" in the body of the class causes these wrappers to be
 * added as members.
 */
#define ADD_SERIALIZE_METHODS                                         \
    template<typename Stream>                                         \
    void Serialize(Stream& s) const {                                 \
        NCONST_PTR(this)->SerializationOp(s, CSerActionSerialize());  \
    }                                                                 \
    template<typename Stream>                                         \
    void Unserialize(Stream& s) {                                     \
        SerializationOp(s, CSerActionUnserialize());                  \
    }

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
        SerializationOps(*this, s, CSerActionSerialize());                          \
    }                                                                               \
    template<typename Stream>                                                       \
    void Unserialize(Stream& s)                                                     \
    {                                                                               \
        static_assert(std::is_same<cls&, decltype(*this)>::value, "Unserialize type mismatch"); \
        SerializationOps(*this, s, CSerActionUnserialize());                        \
    }                                                                               \
    template<typename Stream, typename Type, typename Operation>                    \
    static inline void SerializationOps(Type& obj, Stream& s, Operation ser_action) \


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
template<typename Stream> inline void Serialize(Stream& s, float a   ) { ser_writedata32(s, ser_float_to_uint32(a)); }
template<typename Stream> inline void Serialize(Stream& s, double a  ) { ser_writedata64(s, ser_double_to_uint64(a)); }
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
template<typename Stream> inline void Unserialize(Stream& s, float& a   ) { a = ser_uint32_to_float(ser_readdata32(s)); }
template<typename Stream> inline void Unserialize(Stream& s, double& a  ) { a = ser_uint64_to_double(ser_readdata64(s)); }
template<typename Stream, int N> inline void Unserialize(Stream& s, char (&a)[N]) { s.read(a, N); }
template<typename Stream, int N> inline void Unserialize(Stream& s, unsigned char (&a)[N]) { s.read(CharCast(a), N); }
template<typename Stream> inline void Unserialize(Stream& s, Span<unsigned char>& span) { s.read(CharCast(span.data()), span.size()); }

template<typename Stream> inline void Serialize(Stream& s, bool a)    { char f=a; ser_writedata8(s, f); }
template<typename Stream> inline void Unserialize(Stream& s, bool& a) { char f=ser_readdata8(s); a=f; }

template <typename T> size_t GetSerializeSize(const T& t, int nType, int nVersion = 0);
template <typename S, typename T> size_t GetSerializeSize(const S& s, const T& t);





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
    else if (nSize <= std::numeric_limits<unsigned short>::max()) return sizeof(unsigned char) + sizeof(unsigned short);
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
    else if (nSize <= std::numeric_limits<unsigned short>::max())
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

template<typename Stream>
uint64_t ReadCompactSize(Stream& is)
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
    if (nSizeRet > (uint64_t)MAX_SIZE)
        throw std::ios_base::failure("ReadCompactSize(): size too large");
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

#define FIXEDBITSET(obj, size) CFixedBitSet(REF(obj), (size))
#define DYNBITSET(obj) CDynamicBitSet(REF(obj))
#define FIXEDVARINTSBITSET(obj, size) CFixedVarIntsBitSet(REF(obj), (size))
#define AUTOBITSET(obj, size) CAutoBitSet(REF(obj), (size))
#define VARINT(obj, ...) Using<VarIntFormatter<__VA_ARGS__>>(obj)
#define COMPACTSIZE(obj) CCompactSize(REF(obj))
#define LIMITED_STRING(obj,n) LimitedString< n >(REF(obj))

class CFixedBitSet
{
protected:
    std::vector<bool>& vec;
    size_t size;

public:
    CFixedBitSet(std::vector<bool>& vecIn, size_t sizeIn) : vec(vecIn), size(sizeIn) {}

    template<typename Stream>
    void Serialize(Stream& s) const
    {
        std::vector<unsigned char> vBytes((size + 7) / 8);
        size_t ms = std::min(size, vec.size());
        for (size_t p = 0; p < ms; p++)
            vBytes[p / 8] |= vec[p] << (p % 8);
        s.write((char*)vBytes.data(), vBytes.size());
    }

    template<typename Stream>
    void Unserialize(Stream& s)
    {
        vec.resize(size);

        std::vector<unsigned char> vBytes((size + 7) / 8);
        s.read((char*)vBytes.data(), vBytes.size());
        for (size_t p = 0; p < size; p++)
            vec[p] = (vBytes[p / 8] & (1 << (p % 8))) != 0;
        if (vBytes.size() * 8 != size) {
            size_t rem = vBytes.size() * 8 - size;
            uint8_t m = ~(uint8_t)(0xff >> rem);
            if (vBytes[vBytes.size() - 1] & m) {
                throw std::ios_base::failure("Out-of-range bits set");
            }
        }
    }
};

class CDynamicBitSet
{
protected:
    std::vector<bool>& vec;

public:
    explicit CDynamicBitSet(std::vector<bool>& vecIn) : vec(vecIn) {}

    template<typename Stream>
    void Serialize(Stream& s) const
    {
        WriteCompactSize(s, vec.size());
        CFixedBitSet(REF(vec), vec.size()).Serialize(s);
    }

    template<typename Stream>
    void Unserialize(Stream& s)
    {
        vec.resize(ReadCompactSize(s));
        CFixedBitSet(vec, vec.size()).Unserialize(s);
    }
};

/**
 * Stores a fixed size bitset as a series of VarInts. Each VarInt is an offset from the last entry and the sum of the
 * last entry and the offset gives an index into the bitset for a set bit. The series of VarInts ends with a 0.
 */
class CFixedVarIntsBitSet
{
protected:
    std::vector<bool>& vec;
    size_t size;

public:
    CFixedVarIntsBitSet(std::vector<bool>& vecIn, size_t sizeIn) : vec(vecIn), size(sizeIn) {}

    template<typename Stream>
    void Serialize(Stream& s) const
    {
        int32_t last = -1;
        for (int32_t i = 0; i < (int32_t)vec.size(); i++) {
            if (vec[i]) {
                WriteVarInt<Stream, VarIntMode::DEFAULT, uint32_t>(s, (uint32_t)(i - last));
                last = i;
            }
        }
        WriteVarInt<Stream, VarIntMode::DEFAULT, uint32_t>(s, 0); // stopper
    }

    template<typename Stream>
    void Unserialize(Stream& s)
    {
        vec.assign(size, false);

        int32_t last = -1;
        while(true) {
            uint32_t offset = ReadVarInt<Stream, VarIntMode::DEFAULT, uint32_t>(s);
            if (offset == 0) {
                break;
            }
            int32_t idx = last + offset;
            if (idx >= size) {
                throw std::ios_base::failure("out of bounds index");
            }
            if (last != -1 && idx <= last) {
                throw std::ios_base::failure("offset overflow");
            }
            vec[idx] = true;
            last = idx;
        }
    }
};

/**
 * Serializes either as a CFixedBitSet or CFixedVarIntsBitSet, depending on which would give a smaller size
 */
class CAutoBitSet
{
protected:
    std::vector<bool>& vec;
    size_t size;

public:
    explicit CAutoBitSet(std::vector<bool>& vecIn, size_t sizeIn) : vec(vecIn), size(sizeIn) {}

    template<typename Stream>
    void Serialize(Stream& s) const
    {
        assert(vec.size() == size);

        size_t size1 = ::GetSerializeSize(s, CFixedBitSet(vec, size));
        size_t size2 = ::GetSerializeSize(s, CFixedVarIntsBitSet(vec, size));

        if (size1 < size2) {
            ser_writedata8(s, 0);
            s << FIXEDBITSET(vec, vec.size());
        } else {
            ser_writedata8(s, 1);
            s << FIXEDVARINTSBITSET(vec, vec.size());
        }
    }

    template<typename Stream>
    void Unserialize(Stream& s)
    {
        uint8_t isVarInts = ser_readdata8(s);
        if (isVarInts != 0 && isVarInts != 1) {
            throw std::ios_base::failure("invalid value for isVarInts byte");
        }

        if (!isVarInts) {
            s >> FIXEDBITSET(vec, size);
        } else {
            s >> FIXEDVARINTSBITSET(vec, size);
        }
    }
};

/** Serialization wrapper class for integers in VarInt format. */
template<VarIntMode Mode=VarIntMode::DEFAULT>
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

/** Serialization wrapper class for big-endian integers.
 *
 * Use this wrapper around integer types that are stored in memory in native
 * byte order, but serialized in big endian notation. This is only intended
 * to implement serializers that are compatible with existing formats, and
 * its use is not recommended for new data structures.
 *
 * Only 16-bit types are supported for now.
 */
template<typename I>
class BigEndian
{
protected:
    I& m_val;
public:
    explicit BigEndian(I& val) : m_val(val)
    {
        static_assert(std::is_unsigned<I>::value, "BigEndian type must be unsigned integer");
        static_assert(sizeof(I) == 2 && std::numeric_limits<I>::min() == 0 && std::numeric_limits<I>::max() == std::numeric_limits<uint16_t>::max(), "Unsupported BigEndian size");
    }

    template<typename Stream>
    void Serialize(Stream& s) const
    {
        ser_writedata16be(s, m_val);
    }

    template<typename Stream>
    void Unserialize(Stream& s)
    {
        m_val = ser_readdata16be(s);
    }
};

class CCompactSize
{
protected:
    uint64_t &n;
public:
    explicit CCompactSize(uint64_t& nIn) : n(nIn) { }

    unsigned int GetSerializeSize() const {
        return GetSizeOfCompactSize(n);
    }

    template<typename Stream>
    void Serialize(Stream &s) const {
        WriteCompactSize<Stream>(s, n);
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        n = ReadCompactSize<Stream>(s);
    }
};

template<size_t Limit>
class LimitedString
{
protected:
    std::string& string;
public:
    explicit LimitedString(std::string& _string) : string(_string) {}

    template<typename Stream>
    void Unserialize(Stream& s)
    {
        size_t size = ReadCompactSize(s);
        if (size > Limit) {
            throw std::ios_base::failure("String length limit exceeded");
        }
        string.resize(size);
        if (size != 0)
            s.read((char*)string.data(), size);
    }

    template<typename Stream>
    void Serialize(Stream& s) const
    {
        WriteCompactSize(s, string.size());
        if (!string.empty())
            s.write((char*)string.data(), string.size());
    }
};

template<typename I>
BigEndian<I> WrapBigEndian(I& n) { return BigEndian<I>(n); }

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
 * exposes a value_type, size, reserve, push_back, and const iterators.
 */
template<class Formatter>
struct VectorFormatter
{
    template<typename Stream, typename V>
    void Ser(Stream& s, const V& v)
    {
        WriteCompactSize(s, v.size());
        for (const typename V::value_type& elem : v) {
            s << Using<Formatter>(elem);
        }
    }

    template<typename Stream, typename V>
    void Unser(Stream& s, V& v)
    {
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
                typename V::value_type val;
                s >> Using<Formatter>(val);
                v.push_back(std::move(val));
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
 * pair
 */
template<typename Stream, typename... Elements> void Serialize(Stream& os, const std::tuple<Elements...>& item);
template<typename Stream, typename... Elements> void Unserialize(Stream& is, std::tuple<Elements...>& item);

/**
 * map
 */
template<typename Stream, typename K, typename T, typename Pred, typename A> void Serialize(Stream& os, const std::map<K, T, Pred, A>& m);
template<typename Stream, typename K, typename T, typename Pred, typename A> void Unserialize(Stream& is, std::map<K, T, Pred, A>& m);
template<typename Stream, typename K, typename T, typename Hash, typename Pred, typename A> void Serialize(Stream& os, const std::unordered_map<K, T, Hash, Pred, A>& m);
template<typename Stream, typename K, typename T, typename Hash, typename Pred, typename A> void Unserialize(Stream& is, std::unordered_map<K, T, Hash, Pred, A>& m);

/**
 * set
 */
template<typename Stream, typename K, typename Pred, typename A> void Serialize(Stream& os, const std::set<K, Pred, A>& m);
template<typename Stream, typename K, typename Pred, typename A> void Unserialize(Stream& is, std::set<K, Pred, A>& m);
template<typename Stream, typename K, typename Hash, typename Pred, typename A> void Serialize(Stream& os, const std::unordered_set<K, Hash, Pred, A>& m);
template<typename Stream, typename K, typename Hash, typename Pred, typename A> void Unserialize(Stream& is, std::unordered_set<K, Hash, Pred, A>& m);

/**
 * shared_ptr
 */
template<typename Stream, typename T> void Serialize(Stream& os, const std::shared_ptr<T>& p);
template<typename Stream, typename T> void Unserialize(Stream& os, std::shared_ptr<T>& p);

/**
 * unique_ptr
 */
template<typename Stream, typename T> void Serialize(Stream& os, const std::unique_ptr<const T>& p);
template<typename Stream, typename T> void Unserialize(Stream& os, std::unique_ptr<const T>& p);



/**
 * If none of the specialized versions above matched and T is a class, default to calling member function.
 */
template<typename Stream, typename T, typename std::enable_if<std::is_class<T>::value>::type* = nullptr>
inline void Serialize(Stream& os, const T& a)
{
    a.Serialize(os);
}

template<typename Stream, typename T, typename std::enable_if<std::is_class<std::remove_reference<T> >::value>::type* = nullptr>
inline void Unserialize(Stream& is, T&& a)
{
    a.Unserialize(is);
}

/**
 * If none of the specialized versions above matched and T is an enum, default to calling
 * Serialize/Unserialze with the underlying type. This is only allowed when a specialized struct of is_serializable_enum<Enum>
 * is found which derives from std::true_type. This is to ensure that enums are not serialized with the wrong type by
 * accident.
 */

template<typename T> struct is_serializable_enum;
template<typename T> struct is_serializable_enum : std::false_type {};

template<typename Stream, typename T, typename std::enable_if<std::is_enum<T>::value>::type* = nullptr>
inline void Serialize(Stream& s, const T& a )
{
    // If you ever get into this situation, it usaully means you forgot to declare is_serializable_enum for the desired enum type
    static_assert(is_serializable_enum<T>::value, "Missing declararion of is_serializable_enum");

    typedef typename std::underlying_type<T>::type T2;
    T2 b = (T2)a;
    Serialize(s, b);
}

template<typename Stream, typename T, typename std::enable_if<std::is_enum<T>::value>::type* = nullptr>
inline void Unserialize(Stream& s, T& a )
{
    // If you ever get into this situation, it usaully means you forgot to declare is_serializable_enum for the desired enum type
    static_assert(is_serializable_enum<T>::value, "Missing declararion of is_serializable_enum");

    typedef typename std::underlying_type<T>::type T2;
    T2 b;
    Unserialize(s, b);
    a = (T)b;
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
 * tuple
 */
template<typename Stream, int index, typename... Ts>
struct SerializeTuple {
    void operator() (Stream&s, std::tuple<Ts...>& t) {
        SerializeTuple<Stream, index - 1, Ts...>{}(s, t);
        s << std::get<index>(t);
    }
};

template<typename Stream, typename... Ts>
struct SerializeTuple<Stream, 0, Ts...> {
    void operator() (Stream&s, std::tuple<Ts...>& t) {
        s << std::get<0>(t);
    }
};

template<typename Stream, int index, typename... Ts>
struct DeserializeTuple {
    void operator() (Stream&s, std::tuple<Ts...>& t) {
        DeserializeTuple<Stream, index - 1, Ts...>{}(s, t);
        s >> std::get<index>(t);
    }
};

template<typename Stream, typename... Ts>
struct DeserializeTuple<Stream, 0, Ts...> {
    void operator() (Stream&s, std::tuple<Ts...>& t) {
        s >> std::get<0>(t);
    }
};


template<typename Stream, typename... Elements>
void Serialize(Stream& os, const std::tuple<Elements...>& item)
{
    const auto size = std::tuple_size<std::tuple<Elements...>>::value;
    SerializeTuple<Stream, size - 1, Elements...>{}(os, const_cast<std::tuple<Elements...>&>(item));
}

template<typename Stream, typename... Elements>
void Unserialize(Stream& is, std::tuple<Elements...>& item)
{
    const auto size = std::tuple_size<std::tuple<Elements...>>::value;
    DeserializeTuple<Stream, size - 1, Elements...>{}(is, item);
}


/**
 * map
 */
template<typename Stream, typename Map>
void SerializeMap(Stream& os, const Map& m)
{
    WriteCompactSize(os, m.size());
    for (const auto& entry : m)
        Serialize(os, entry);
}

template<typename Stream, typename Map>
void UnserializeMap(Stream& is, Map& m)
{
    m.clear();
    unsigned int nSize = ReadCompactSize(is);
    auto mi = m.begin();
    for (unsigned int i = 0; i < nSize; i++)
    {
        std::pair<typename std::remove_const<typename Map::key_type>::type, typename std::remove_const<typename Map::mapped_type>::type> item;
        Unserialize(is, item);
        mi = m.insert(mi, item);
    }
}

template<typename Stream, typename K, typename T, typename Pred, typename A>
void Serialize(Stream& os, const std::map<K, T, Pred, A>& m)
{
    SerializeMap(os, m);
}

template<typename Stream, typename K, typename T, typename Pred, typename A>
void Unserialize(Stream& is, std::map<K, T, Pred, A>& m)
{
    UnserializeMap(is, m);
}

template<typename Stream, typename K, typename T, typename Hash, typename Pred, typename A>
void Serialize(Stream& os, const std::unordered_map<K, T, Hash, Pred, A>& m)
{
    SerializeMap(os, m);
}

template<typename Stream, typename K, typename T, typename Hash, typename Pred, typename A>
void Unserialize(Stream& is, std::unordered_map<K, T, Hash, Pred, A>& m)
{
    UnserializeMap(is, m);
}

/**
 * set
 */

template<typename Stream, typename Set>
void SerializeSet(Stream& os, const Set& m)
{
    WriteCompactSize(os, m.size());
    for (auto it = m.begin(); it != m.end(); ++it)
        Serialize(os, (*it));
}

template<typename Stream, typename Set>
void UnserializeSet(Stream& is, Set& m)
{
    m.clear();
    unsigned int nSize = ReadCompactSize(is);
    auto it = m.begin();
    for (unsigned int i = 0; i < nSize; i++)
    {
        typename std::remove_const<typename Set::key_type>::type key;
        Unserialize(is, key);
        it = m.insert(it, key);
    }
}

template<typename Stream, typename K, typename Pred, typename A>
void Serialize(Stream& os, const std::set<K, Pred, A>& m)
{
    SerializeSet(os, m);
}

template<typename Stream, typename K, typename Pred, typename A>
void Unserialize(Stream& is, std::set<K, Pred, A>& m)
{
    UnserializeSet(is, m);
}

template<typename Stream, typename K, typename Hash, typename Pred, typename A>
void Serialize(Stream& os, const std::unordered_set<K, Hash, Pred, A>& m)
{
    SerializeSet(os, m);
}

template<typename Stream, typename K, typename Hash, typename Pred, typename A>
void Unserialize(Stream& is, std::unordered_set<K, Hash, Pred, A>& m)
{
    UnserializeSet(is, m);
}

/**
 * list
 */
template<typename Stream, typename T, typename A>
void Serialize(Stream& os, const std::list<T, A>& l)
{
    WriteCompactSize(os, l.size());
    for (typename std::list<T, A>::const_iterator it = l.begin(); it != l.end(); ++it)
        Serialize(os, (*it));
}

template<typename Stream, typename T, typename A>
void Unserialize(Stream& is, std::list<T, A>& l)
{
    l.clear();
    unsigned int nSize = ReadCompactSize(is);
    for (unsigned int i = 0; i < nSize; i++)
    {
        T val;
        Unserialize(is, val);
        l.push_back(val);
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
Serialize(Stream& os, const std::shared_ptr<T>& p)
{
    Serialize(os, *p);
}

template<typename Stream, typename T>
void Unserialize(Stream& is, std::shared_ptr<T>& p)
{
    p = std::make_shared<T>(deserialize, is);
}



/**
 * Support for ADD_SERIALIZE_METHODS and READWRITE macro
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

    const int nType;
    const int nVersion;
public:
    CSizeComputer(int nTypeIn, int nVersionIn) : nSize(0), nType(nTypeIn), nVersion(nVersionIn) {}

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
    int GetType() const { return nType; }
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
size_t GetSerializeSize(const T& t, int nType, int nVersion)
{
    return (CSizeComputer(nType, nVersion) << t).size();
}

template <typename S, typename T>
size_t GetSerializeSize(const S& s, const T& t)
{
    return (CSizeComputer(s.GetType(), s.GetVersion()) << t).size();
}

#endif // BITCOIN_SERIALIZE_H
