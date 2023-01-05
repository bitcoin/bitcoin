// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/mcl/mcl_scalar.h>

MclScalar::MclScalar(const int64_t& n)
{
    mclBnFr_setInt(&m_fr, n);  // this takes int64_t
}

MclScalar::MclScalar(const std::vector<uint8_t> &v)
{
    MclScalar::SetVch(v);
}

MclScalar::MclScalar(const mclBnFr& other_fr)
{
    m_fr = other_fr;
}

MclScalar::MclScalar(const uint256& n)
{
    // uint256 deserialization is big-endian
    mclBnFr_setBigEndianMod(&m_fr, n.data(), 32);
}

MclScalar::MclScalar(const std::string& s, int radix)
{
    auto r = mclBnFr_setStr(&m_fr, s.c_str(), s.length(), radix);
    if (r == -1) {
        throw std::runtime_error(std::string("Failed to instantiate Scalar from '") + s);
    }
}

void MclScalar::Init()
{
    MclInitializer::Init();
}

MclScalar MclScalar::operator+(const MclScalar &rhs) const
{
    MclScalar ret;
    mclBnFr_add(&ret.m_fr, &m_fr, &rhs.m_fr);
    return ret;
}

MclScalar MclScalar::operator-(const MclScalar &rhs) const
{
    MclScalar ret;
    mclBnFr_sub(&ret.m_fr, &m_fr, &rhs.m_fr);
    return ret;
}

MclScalar MclScalar::operator*(const MclScalar &rhs) const
{
    MclScalar ret;
    mclBnFr_mul(&ret.m_fr, &m_fr, &rhs.m_fr);
    return ret;
}

MclScalar MclScalar::operator/(const MclScalar &rhs) const
{
    MclScalar ret;
    mclBnFr_div(&ret.m_fr, &m_fr, &rhs.m_fr);
    return ret;
}

MclScalar MclScalar::ApplyBitwiseOp(const MclScalar& a, const MclScalar& b,
    std::function<uint8_t(uint8_t, uint8_t)> op) const
{
    MclScalar ret;
    auto a_vec = a.GetVch();
    auto b_vec = b.GetVch();

    // If sizes are the same, bVec becomes longer and aVec becomes shorter
    auto& longer = a_vec.size() > b_vec.size() ? a_vec : b_vec;
    auto& shorter = b_vec.size() < a_vec.size() ? b_vec : a_vec;

    std::vector<uint8_t> c_vec(longer.size());

    for (size_t i = 0; i < shorter.size(); i++) {
        uint8_t l = longer[i];
        uint8_t r = shorter[i];
        c_vec[i] = op(l, r);
    }

    // Does nothing if sizes are the same
    for (size_t i = shorter.size(); i < longer.size(); i++) {
        c_vec[i] = op(longer[i], 0);
    }

    ret.SetVch(c_vec);

    return ret;
}

MclScalar MclScalar::operator|(const MclScalar &rhs) const
{
    auto op = [](uint8_t a, uint8_t b) -> uint8_t { return a | b; };
    return ApplyBitwiseOp(*this, rhs, op);
}

MclScalar MclScalar::operator^(const MclScalar &rhs) const
{
    auto op = [](uint8_t a, uint8_t b) -> uint8_t { return a ^ b; };
    return ApplyBitwiseOp(*this, rhs, op);
}

MclScalar MclScalar::operator&(const MclScalar &rhs) const
{
    auto op = [](uint8_t a, uint8_t b) -> uint8_t { return a & b; };
    return ApplyBitwiseOp(*this, rhs, op);
}

MclScalar MclScalar::operator~() const
{
    // Getting complement of lower 8 bytes only since when 32-byte buffer is fully complemented,
    // mclBrFr_deserialize returns undesired result
    const int64_t n_complement_scalar = (int64_t) ~GetUint64();
    MclScalar ret(n_complement_scalar);

    return ret;
}

MclScalar MclScalar::operator<<(const uint32_t& shift) const
{
    mclBnFr next;
    mclBnFr prev = m_fr;
    for (size_t i = 0; i < shift; ++i) {
        mclBnFr_add(&next, &prev, &prev);
        prev = next;
    }
    MclScalar ret(prev);

    return ret;
}

MclScalar MclScalar::operator>>(const uint32_t& shift) const
{
    mclBnFr one;
    mclBnFr two;
    mclBnFr_setInt(&one, 1);
    mclBnFr_setInt(&two, 2);

    mclBnFr temp = m_fr;
    uint32_t n = shift;

    while (n > 0) {
        if (mclBnFr_isOdd(&temp) != 0) {
            mclBnFr_sub(&temp, &temp, &one);
        }
        mclBnFr_div(&temp, &temp, &two);
        --n;
    }
    MclScalar ret(temp);
    return ret;
}

void MclScalar::operator=(const int64_t& n)
{
    mclBnFr_setInt(&m_fr, n);
}

bool MclScalar::operator==(const int32_t& rhs) const
{
    MclScalar temp;
    temp = rhs;
    return mclBnFr_isEqual(&m_fr, &temp.m_fr);
}

bool MclScalar::operator==(const MclScalar &rhs) const
{
    return mclBnFr_isEqual(&m_fr, &rhs.m_fr);
}

bool MclScalar::operator!=(const int &b) const
{
    return !operator==(b);
}

bool MclScalar::operator!=(const MclScalar &b) const
{
    return !operator==(b);
}

mclBnFr MclScalar::Underlying() const
{
    return m_fr;
}

bool MclScalar::IsValid() const
{
    return mclBnFr_isValid(&m_fr) == 1;
}

MclScalar MclScalar::Invert() const
{
    if (mclBnFr_isZero(&m_fr) == 1) {
        throw std::runtime_error("Inverse of zero is undefined");
    }
    MclScalar temp;
    mclBnFr_inv(&temp.m_fr, &m_fr);
    return temp;
}

MclScalar MclScalar::Negate() const
{
    MclScalar temp;
    mclBnFr_neg(&temp.m_fr, &m_fr);
    return temp;
}

MclScalar MclScalar::Square() const
{
    MclScalar temp;
    mclBnFr_sqr(&temp.m_fr, &m_fr);
    return temp;
}

MclScalar MclScalar::Cube() const
{
    MclScalar temp(m_fr);
    temp = temp * temp.Square();
    return temp;
}

MclScalar MclScalar::Pow(const MclScalar& n) const
{
    // A variant of double-and-add method
    MclScalar temp(1);
    mclBnFr bit_val;
    bit_val = m_fr;
    auto bits = n.ToBinaryVec();

    for (auto it = bits.rbegin(); it != bits.rend(); ++it) {
        MclScalar s(bit_val);
        if (*it) {
            mclBnFr_mul(&temp.m_fr, &temp.m_fr, &bit_val);
        }
        mclBnFr_mul(&bit_val, &bit_val, &bit_val);
    }
    return temp;
}

MclScalar MclScalar::Rand(bool exclude_zero)
{
    MclScalar temp;

    while (true) {
        if (mclBnFr_setByCSPRNG(&temp.m_fr) != 0) {
            throw std::runtime_error(std::string("Failed to generate random number"));
        }
        if (!exclude_zero || mclBnFr_isZero(&temp.m_fr) != 1) break;
    }
    return temp;
}

uint64_t MclScalar::GetUint64() const
{
    uint64_t ret = 0;
    std::vector<uint8_t> vch = GetVch();
    for (auto i = 0; i < 8; ++i) {
        ret |= (uint64_t) vch[vch.size() - 1 - i] << i * 8;
    }
    return ret;
}

std::vector<uint8_t> MclScalar::GetVch(const bool trim_preceeding_zeros) const
{
    auto seri_size = MclScalar::GetSerializeSize();
    std::vector<uint8_t> vec(seri_size);
    if (mclBnFr_serialize(&vec[0], seri_size, &m_fr) == 0) {
        throw std::runtime_error(std::string("Serialization failed"));
    }
    if (!trim_preceeding_zeros) return vec;

    std::vector<uint8_t> trimmed_vec;

    bool take_char = false;
    for (auto c: vec) {
        if (!take_char && c != '\0') take_char = true;
        if (take_char) trimmed_vec.push_back(c);
    }
    return trimmed_vec;
}

void MclScalar::SetVch(const std::vector<uint8_t> &v)
{
    if (v.size() == 0) {
        mclBnFr x;
        mclBnFr_clear(&x);
        m_fr = x;
    } else {
        if (mclBnFr_setBigEndianMod(&m_fr, &v[0], v.size()) == -1) {
            throw std::runtime_error(std::string("Failed to setBigEndianMod vector"));
        }
    }
}

void MclScalar::SetPow2(const uint32_t& n)
{
    uint32_t i = n;
    MclScalar temp = 1;

    while (i != 0) {
        temp = temp * 2;
        --i;
    }
    m_fr = temp.m_fr;
}

uint256 MclScalar::GetHashWithSalt(const uint64_t& salt) const
{
    CHashWriter hasher(0,0);
    hasher << *this;
    hasher << salt;
    return hasher.GetHash();
}

std::string MclScalar::GetString(const int8_t& radix) const
{
    char str[1024];

    if (mclBnFr_getStr(str, sizeof(str), &m_fr, radix) == 0) {
        throw std::runtime_error(std::string("Failed to get string representation of mclBnFr"));
    }
    return std::string(str);
}

std::vector<bool> MclScalar::ToBinaryVec() const
{
    auto bitStr = GetString(2);
    std::vector<bool> vec;
    for (auto& c : bitStr) {
        vec.push_back(c == '0' ? 0 : 1);
    }
    return vec;
}

/**
 * Since GetVch returns 32-byte vector, maximum bit index is 8 * 32 - 1 = 255
 */
bool MclScalar::GetSeriBit(const uint8_t& n) const
{
    std::vector<uint8_t> vch = GetVch();
    assert(vch.size() == SERIALIZATION_SIZE);

    const uint8_t vchIdx = 31 - n / 8;  // vch is little-endian
    const uint8_t bitIdx = n % 8;
    const uint8_t mask = 1 << bitIdx;
    const bool bit = (vch[vchIdx] & mask) != 0;

    return bit;
}

unsigned int MclScalar::GetSerializeSize() const
{
    return SERIALIZATION_SIZE;
}

template <typename Stream>
void MclScalar::Serialize(Stream& s) const
{
    ::Serialize(s, GetVch());
}

template <typename Stream>
void MclScalar::Unserialize(Stream& s)
{
    std::vector<uint8_t> vch;
    ::Unserialize(s, vch);
    SetVch(vch);
}
