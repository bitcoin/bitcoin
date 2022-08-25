// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/g1point.h>
#include <numeric>

mclBnG1 G1Point::m_g;
boost::mutex G1Point::m_init_mutex;

G1Point::G1Point()
{
    mclBnG1_clear(&m_p);
}

G1Point::G1Point(const std::vector<uint8_t>& v)
{
    G1Point::SetVch(v);
}

G1Point::G1Point(const mclBnG1& q)
{
    m_p = q;
}

G1Point::G1Point(const uint256& b)
{
    // Not using G1Point::MapToG1 since uint256 deserialization is big-endian
    G1Point temp;
    mclBnFp v;
    if (mclBnFp_setBigEndianMod(&v, b.data(), b.size()) != 0) {
        throw std::runtime_error("G1Point(uint256): mclBnFp_setLittleEndianMod failed");
    }
    if (mclBnFp_mapToG1(&temp.m_p, &v) != 0) {
        throw std::runtime_error("G1Point(uint256): mclBnFp_mapToG1 failed");
    }
    m_p = temp.m_p;
}

void G1Point::Init()
{
    boost::lock_guard<boost::mutex> lock(G1Point::m_init_mutex);
    static bool is_initialized = false;
    if (is_initialized) return;

    MclInitializer::Init();
    mclBnG1 g;
    const char* serialized_g = "1 3685416753713387016781088315183077757961620795782546409894578378688607592378376318836054947676345821548104185464507 1339506544944476473020471379941921221584933875938349620426543736416511423956333506472724655353366534992391756441569";
    if (mclBnG1_setStr(&g, serialized_g, strlen(serialized_g), 10) == -1) {
        throw std::runtime_error("G1Point::Init(): mclBnG1_setStr failed");
    }
    G1Point::m_g = g;
    is_initialized = true;
}

G1Point G1Point::operator=(const mclBnG1& q)
{
    m_p = q;
    return *this;
}

G1Point G1Point::operator+(const G1Point& b) const
{
    G1Point ret;
    mclBnG1_add(&ret.m_p, &m_p, &b.m_p);
    return ret;
}

G1Point G1Point::operator-(const G1Point& b) const
{
    G1Point ret;
    mclBnG1_sub(&ret.m_p, &m_p, &b.m_p);
    return ret;
}

G1Point G1Point::operator*(const Scalar& b) const
{
    G1Point ret;
    mclBnG1_mul(&ret.m_p, &m_p, &b.m_fr);
    return ret;
}

G1Point G1Point::Double() const
{
    G1Point temp;
    mclBnG1_dbl(&temp.m_p, &m_p);
    return temp;
}

G1Point G1Point::GetBasePoint()
{
    G1Point g(G1Point::m_g);
    return g;
}

G1Point G1Point::MapToG1(const std::vector<uint8_t>& vec, const Endianness e)
{
    if (vec.size() == 0) {
        throw std::runtime_error("G1Point::MapToG1(): input vector is empty");
    }
    G1Point temp;
    mclBnFp v;
    if (e == Endianness::Little) {
        if (mclBnFp_setLittleEndianMod(&v, &vec[0], vec.size()) != 0) {
            throw std::runtime_error("G1Point::MapToG1(): mclBnFp_setLittleEndianMod failed");
        }
    } else {
        if (mclBnFp_setBigEndianMod(&v, &vec[0], vec.size()) != 0) {
            throw std::runtime_error("G1Point::MapToG1(): mclBnFp_setBigEndianMod failed");
        }
    }

    if (mclBnFp_mapToG1(&temp.m_p, &v) != 0) {
        throw std::runtime_error("G1Point::MapToG1(): mclBnFp_mapToG1 failed");
    }
    return temp;
}

G1Point G1Point::MapToG1(const std::string& s, const Endianness e)
{
    std::vector<uint8_t> vec(s.begin(), s.end());
    return MapToG1(vec);
}

G1Point G1Point::HashAndMap(const std::vector<uint8_t>& vec)
{
    mclBnG1 p;
    if (mclBnG1_hashAndMapTo(&p, &vec[0], vec.size()) != 0) {
        throw std::runtime_error("G1Point::HashAndMap(): mclBnG1_hashAndMapTo failed");
    }
    G1Point temp(p);
    return temp;
}

G1Point G1Point::MulVec(const std::vector<mclBnG1>& g_vec, const std::vector<mclBnFr>& s_vec)
{
    if (g_vec.size() != s_vec.size()) {
        throw std::runtime_error("G1Point::MulVec(): sizes of g_vec and s_vec must be equial");
    }

    G1Point ret;
    const auto vec_count = g_vec.size();
    mclBnG1_mulVec(&ret.m_p, g_vec.data(), s_vec.data(), vec_count);

    return ret;
}

bool G1Point::operator==(const G1Point& b) const
{
    return mclBnG1_isEqual(&m_p, &b.m_p);
}

bool G1Point::operator!=(const G1Point& b) const
{
    return !operator==(b);
}

G1Point G1Point::Rand()
{
    auto g = GetBasePoint();
    return g * Scalar::Rand();
}

bool G1Point::IsUnity() const
{
    return mclBnG1_isZero(&m_p);
}

std::vector<uint8_t> G1Point::GetVch() const
{
    std::vector<uint8_t> b(WIDTH);
    if (mclBnG1_serialize(&b[0], WIDTH, &m_p) == 0) {
        throw std::runtime_error("G1Point::GetVch(): mclBnG1_serialize failed");
    }
    return b;
}

void G1Point::SetVch(const std::vector<uint8_t>& b)
{
    if (mclBnG1_deserialize(&m_p, &b[0], b.size()) == 0) {
        throw std::runtime_error("G1Point::SetVch(): mclBnG1_deserialize failed");
    }
}

std::string G1Point::GetString(const int& radix) const
{
    char str[1024];
    if (mclBnG1_getStr(str, sizeof(str), &m_p, radix) == 0) {
        throw std::runtime_error("G1Point::GetString(): mclBnG1_getStr failed");
    }
    return std::string(str);
}

unsigned int G1Point::GetSerializeSize() const
{
    return ::GetSerializeSize(GetVch());
}
