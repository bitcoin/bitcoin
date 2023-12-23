// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/mcl/mcl_g1point.h>
#include <numeric>
#include <streams.h>

MclG1Point::MclG1Point()
{
    // Replacement of mclBnG1_clear to avoid segfault in static context
    std::memset(&m_point, 0, sizeof(MclG1Point::Underlying));
}

MclG1Point::MclG1Point(const std::vector<uint8_t>& v)
{
    MclG1Point::SetVch(v);
}

MclG1Point::MclG1Point(const MclG1Point::Underlying& p)
{
    m_point = p;
}

MclG1Point::MclG1Point(const uint256& n)
{
    // Not using MclG1Point::MapToPoint since uint256 deserialization is big-endian
    MclG1Point temp;
    mclBnFp v;
    if (mclBnFp_setBigEndianMod(&v, n.data(), n.size()) != 0) {
        throw std::runtime_error(std::string(__func__) + ": mclBnFp_setLittleEndianMod failed");
    }
    if (mclBnFp_mapToG1(&temp.m_point, &v) != 0) {
        throw std::runtime_error(std::string(__func__) + ": mclBnFp_mapToG1 failed");
    }
    m_point = temp.m_point;
}

const MclG1Point::Underlying& MclG1Point::GetUnderlying() const
{
    return m_point;
}

MclG1Point MclG1Point::operator=(const Underlying& rhs)
{
    m_point = rhs;
    return *this;
}

MclG1Point MclG1Point::operator+(const MclG1Point& rhs) const
{
    MclG1Point ret;
    mclBnG1_add(&ret.m_point, &m_point, &rhs.m_point);
    return ret;
}

MclG1Point MclG1Point::operator-(const MclG1Point& rhs) const
{
    MclG1Point ret;
    mclBnG1_sub(&ret.m_point, &m_point, &rhs.m_point);
    return ret;
}

MclG1Point MclG1Point::operator*(const MclG1Point::Scalar& rhs) const
{
    MclG1Point ret;
    mclBnG1_mul(&ret.m_point, &m_point, &rhs.m_scalar);
    return ret;
}

std::vector<MclG1Point> MclG1Point::operator*(const std::vector<MclG1Point::Scalar>& ss) const
{
    if (ss.size() == 0) {
        throw std::runtime_error(std::string(__func__) + ": Cannot multiply MclG1Point by empty scalar vector");
    }
    std::vector<MclG1Point> ret;

    MclG1Point p = *this;
    for (size_t i = 0; i < ss.size(); ++i) {
        MclG1Point q = p * ss[i];
        ret.push_back(q);
    }
    return ret;
}

MclG1Point MclG1Point::Double() const
{
    MclG1Point temp;
    mclBnG1_dbl(&temp.m_point, &m_point);
    return temp;
}

MclG1Point MclG1Point::GetBasePoint()
{
    using Point = MclG1Point::Underlying;
    static Point* g = nullptr;
    if (g == nullptr) {
        g = new Point();
        auto g_str = "1 3685416753713387016781088315183077757961620795782546409894578378688607592378376318836054947676345821548104185464507 1339506544944476473020471379941921221584933875938349620426543736416511423956333506472724655353366534992391756441569"s;
        if (mclBnG1_setStr(g, g_str.c_str(), g_str.length(), 10) == -1) {
            throw std::runtime_error(std::string(__func__) + ": mclBnG1_setStr failed");
        }
    }
    MclG1Point ret(*g);
    return ret;
}

MclG1Point MclG1Point::MapToPoint(const std::vector<uint8_t>& vec, const Endianness e)
{
    if (vec.size() == 0) {
        throw std::runtime_error(std::string(__func__) + ": Cannot map empty input vector to a point");
    }
    if (vec.size() > sizeof(mclBnFp) * 2) {
        throw std::runtime_error(std::string(__func__) + ": Size of vector must be smaller or equal to the size of mclBnFp * 2");
    }
    MclG1Point temp;
    mclBnFp v;
    if (e == Endianness::Little) {
        if (mclBnFp_setLittleEndianMod(&v, &vec[0], vec.size()) != 0) {
            throw std::runtime_error(std::string(__func__) + ": mclBnFp_setLittleEndianMod failed");
        }
    } else {
        if (mclBnFp_setBigEndianMod(&v, &vec[0], vec.size()) != 0) {
            throw std::runtime_error(std::string(__func__) + ": mclBnFp_setBigEndianMod failed");
        }
    }
    if (mclBnFp_mapToG1(&temp.m_point, &v) != 0) {
        throw std::runtime_error(std::string(__func__) + ": mclBnFp_mapToG1 failed");
    }
    return temp;
}

MclG1Point MclG1Point::MapToPoint(const std::string& s, const Endianness e)
{
    std::vector<uint8_t> vec(s.begin(), s.end());
    return MapToPoint(vec, e);
}

MclG1Point MclG1Point::HashAndMap(const std::vector<uint8_t>& vec)
{
    mclBnG1 p;
    if (mclBnG1_hashAndMapTo(&p, &vec[0], vec.size()) != 0) {
        throw std::runtime_error(std::string(__func__) + ": mclBnG1_hashAndMapTo failed");
    }
    MclG1Point temp(p);
    return temp;
}

bool MclG1Point::operator==(const MclG1Point& rhs) const
{
    return mclBnG1_isEqual(&m_point, &rhs.m_point);
}

bool MclG1Point::operator!=(const MclG1Point& rhs) const
{
    return !operator==(rhs);
}

MclG1Point MclG1Point::Rand()
{
    auto g = GetBasePoint();
    return g * MclScalar::Rand();
}

bool MclG1Point::IsValid() const
{
    return mclBnG1_isValid(&m_point) == 1;
}

bool MclG1Point::IsZero() const
{
    return mclBnG1_isZero(&m_point);
}

std::vector<uint8_t> MclG1Point::GetVch() const
{
    std::vector<uint8_t> b(SERIALIZATION_SIZE);
    if (mclBnG1_serialize(&b[0], SERIALIZATION_SIZE, &m_point) == 0) {
        MclG1Point ret;
        return ret.GetVch();
    }
    return b;
}

bool MclG1Point::SetVch(const std::vector<uint8_t>& b)
{
    if (mclBnG1_deserialize(&m_point, &b[0], b.size()) == 0) {
        mclBnG1 x;
        mclBnG1_clear(&x);
        m_point = x;
        return false;
    }
    return true;
}

std::string MclG1Point::GetString(const uint8_t& radix) const
{
    char str[1024];
    if (mclBnG1_getStr(str, sizeof(str), &m_point, radix) == 0) {
        throw std::runtime_error(std::string(__func__) + ": mclBnG1_getStr failed");
    }
    return std::string(str);
}

MclG1Point::Scalar MclG1Point::GetHashWithSalt(const uint64_t salt) const
{
    HashWriter hasher{};
    hasher << *this;
    hasher << salt;
    MclScalar hash(hasher.GetHash());
    return hash;
}
