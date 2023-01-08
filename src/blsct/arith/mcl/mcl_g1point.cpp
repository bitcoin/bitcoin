// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/mcl/mcl_g1point.h>
#include <numeric>
#include <streams.h>

mclBnG1* MclG1Point::m_g = nullptr;
boost::mutex MclG1Point::m_init_mutex;

MclG1Point::MclG1Point()
{
    mclBnG1_clear(&m_p);
}

MclG1Point::MclG1Point(const std::vector<uint8_t>& v)
{
    MclG1Point::SetVch(v);
}

MclG1Point::MclG1Point(const mclBnG1& q)
{
    m_p = q;
}

MclG1Point::MclG1Point(const uint256& b)
{
    // Not using MclG1Point::MapToG1 since uint256 deserialization is big-endian
    MclG1Point temp;
    mclBnFp v;
    if (mclBnFp_setBigEndianMod(&v, b.data(), b.size()) != 0) {
        throw std::runtime_error("MclG1Point(uint256): mclBnFp_setLittleEndianMod failed");
    }
    if (mclBnFp_mapToG1(&temp.m_p, &v) != 0) {
        throw std::runtime_error("MclG1Point(uint256): mclBnFp_mapToG1 failed");
    }
    m_p = temp.m_p;
}

void MclG1Point::Init()
{
    boost::lock_guard<boost::mutex> lock(MclG1Point::m_init_mutex);
    static bool is_initialized = false;
    if (is_initialized) return;

    MclInitializer::Init();
    mclBnG1* g = new mclBnG1();
    const char* serialized_g = "1 3685416753713387016781088315183077757961620795782546409894578378688607592378376318836054947676345821548104185464507 1339506544944476473020471379941921221584933875938349620426543736416511423956333506472724655353366534992391756441569";
    if (mclBnG1_setStr(g, serialized_g, strlen(serialized_g), 10) == -1) {
        throw std::runtime_error("MclG1Point::Init(): mclBnG1_setStr failed");
    }
    MclG1Point::m_g = g;
    is_initialized = true;
}

void MclG1Point::Dispose()
{
    if (MclG1Point::m_g != nullptr) delete MclG1Point::m_g;
}

mclBnG1 MclG1Point::Underlying() const
{
    return m_p;
}

MclG1Point MclG1Point::operator=(const mclBnG1& q)
{
    m_p = q;
    return *this;
}

MclG1Point MclG1Point::operator+(const MclG1Point& p) const
{
    MclG1Point ret;
    mclBnG1_add(&ret.m_p, &m_p, &p.m_p);
    return ret;
}

MclG1Point MclG1Point::operator-(const MclG1Point& p) const
{
    MclG1Point ret;
    mclBnG1_sub(&ret.m_p, &m_p, &p.m_p);
    return ret;
}

MclG1Point MclG1Point::operator*(const MclScalar& s) const
{
    MclG1Point ret;
    mclBnG1_mul(&ret.m_p, &m_p, &s.m_fr);
    return ret;
}

std::vector<MclG1Point> MclG1Point::operator*(const std::vector<MclScalar>& ss) const
{
    if (ss.size() == 0) {
        throw std::runtime_error("MclG1Point::operator*: cannot multiplyMclG1Point by empty Scalars");
    }
    std::vector<MclG1Point> ret;

    MclG1Point p = *this;
    for(size_t i = 0; i < ss.size(); ++i) {
       MclG1Point q = p * ss[i];
        ret.push_back(q);
    }
    return ret;
}

MclG1Point MclG1Point::Double() const
{
    MclG1Point temp;
    mclBnG1_dbl(&temp.m_p, &m_p);
    return temp;
}

MclG1Point MclG1Point::GetBasePoint()
{
    MclG1Point g(*MclG1Point::m_g);
    return g;
}

MclG1Point MclG1Point::MapToG1(const std::vector<uint8_t>& vec, const Endianness e)
{
    if (vec.size() == 0) {
        throw std::runtime_error("MclG1Point::MapToG1(): input vector is empty");
    }
    MclG1Point temp;
    mclBnFp v;
    if (e == Endianness::Little) {
        if (mclBnFp_setLittleEndianMod(&v, &vec[0], vec.size()) != 0) {
            throw std::runtime_error("MclG1Point::MapToG1(): mclBnFp_setLittleEndianMod failed");
        }
    } else {
        if (mclBnFp_setBigEndianMod(&v, &vec[0], vec.size()) != 0) {
            throw std::runtime_error("MclG1Point::MapToG1(): mclBnFp_setBigEndianMod failed");
        }
    }

    if (mclBnFp_mapToG1(&temp.m_p, &v) != 0) {
        throw std::runtime_error("MclG1Point::MapToG1(): mclBnFp_mapToG1 failed");
    }
    return temp;
}

MclG1Point MclG1Point::MapToG1(const std::string& s, const Endianness e)
{
    std::vector<uint8_t> vec(s.begin(), s.end());
    return MapToG1(vec);
}

MclG1Point MclG1Point::HashAndMap(const std::vector<uint8_t>& vec)
{
    mclBnG1 p;
    if (mclBnG1_hashAndMapTo(&p, &vec[0], vec.size()) != 0) {
        throw std::runtime_error("MclG1Point::HashAndMap(): mclBnG1_hashAndMapTo failed");
    }
    MclG1Point temp(p);
    return temp;
}

bool MclG1Point::operator==(const MclG1Point& b) const
{
    return mclBnG1_isEqual(&m_p, &b.m_p);
}

bool MclG1Point::operator!=(const MclG1Point& b) const
{
    return !operator==(b);
}

MclG1Point MclG1Point::Rand()
{
    auto g = GetBasePoint();
    return g * MclScalar::Rand();
}

bool MclG1Point::IsValid() const
{
    return mclBnG1_isValid(&m_p) == 1;
}

bool MclG1Point::IsUnity() const
{
    return mclBnG1_isZero(&m_p);
}

std::vector<uint8_t> MclG1Point::GetVch() const
{
    std::vector<uint8_t> b(SERIALIZATION_SIZE);
    if (mclBnG1_serialize(&b[0], SERIALIZATION_SIZE, &m_p) == 0) {
        throw std::runtime_error("MclG1Point::GetVch(): mclBnG1_serialize failed");
    }
    return b;
}

void MclG1Point::SetVch(const std::vector<uint8_t>& b)
{
    if (mclBnG1_deserialize(&m_p, &b[0], b.size()) == 0) {
        throw std::runtime_error("MclG1Point::SetVch(): mclBnG1_deserialize failed");
    }
}

std::string MclG1Point::GetString(const uint8_t& radix) const
{
    char str[1024];
    if (mclBnG1_getStr(str, sizeof(str), &m_p, radix) == 0) {
        throw std::runtime_error("MclG1Point::GetString(): mclBnG1_getStr failed");
    }
    return std::string(str);
}

size_t MclG1Point::GetSerializeSize() const
{
    return SERIALIZATION_SIZE;
}

MclScalar MclG1Point::GetHashWithSalt(const uint64_t salt) const
{
    CHashWriter hasher(0,0);
    hasher << *this;
    hasher << salt;
    MclScalar hash(hasher.GetHash());
    return hash;
}

template <typename Stream>
void MclG1Point::Serialize(Stream& s) const
{
    ::Serialize(s, GetVch());
}
template void MclG1Point::Serialize(CDataStream& s) const;
template void MclG1Point::Serialize(CHashWriter& s) const;

template <typename Stream>
void MclG1Point::Unserialize(Stream& s)
{
    std::vector<uint8_t> vch;
    ::Unserialize(s, vch);
    SetVch(vch);
}
template void MclG1Point::Unserialize(CDataStream& s);
