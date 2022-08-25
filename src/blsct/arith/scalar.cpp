// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/scalar.h>

Scalar::Scalar(const int64_t& n)
{
    mclBnFr_setInt(&m_fr, n);
}

Scalar::Scalar(const std::vector<uint8_t> &v)
{
    Scalar::SetVch(v);
}

Scalar::Scalar(const mclBnFr& nFr)
{
    m_fr = nFr;
}

Scalar::Scalar(const uint256& n)
{
    // uint256 deserialization is big-endian
    mclBnFr_setBigEndianMod(&m_fr, n.data(), 32);
}

Scalar::Scalar(const std::string& s, int radix)
{
    auto r = mclBnFr_setStr(&m_fr, s.c_str(), s.length(), radix);
    if (r == -1) {
        throw std::runtime_error(std::string("Failed to instantiate Scalar from '") + s);
    }
}

void Scalar::Init()
{
    MclInitializer::Init();
}

Scalar Scalar::operator+(const Scalar &b) const
{
    Scalar ret;
    mclBnFr_add(&ret.m_fr, &m_fr, &b.m_fr);
    return ret;
}

Scalar Scalar::operator-(const Scalar &b) const
{
    Scalar ret;
    mclBnFr_sub(&ret.m_fr, &m_fr, &b.m_fr);
    return ret;
}

Scalar Scalar::operator*(const Scalar &b) const
{
    Scalar ret;
    mclBnFr_mul(&ret.m_fr, &m_fr, &b.m_fr);
    return ret;
}

Scalar Scalar::operator/(const Scalar &b) const
{
    Scalar ret;
    mclBnFr_div(&ret.m_fr, &m_fr, &b.m_fr);
    return ret;
}

Scalar Scalar::ApplyBitwiseOp(const Scalar& a, const Scalar& b,
    std::function<uint8_t(uint8_t, uint8_t)> op) const
{
    Scalar ret;
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

Scalar Scalar::operator|(const Scalar &b) const
{
    auto op = [](uint8_t a, uint8_t b) -> uint8_t { return a | b; };
    return ApplyBitwiseOp(*this, b, op);
}

Scalar Scalar::operator^(const Scalar &b) const
{
    auto op = [](uint8_t a, uint8_t b) -> uint8_t { return a ^ b; };
    return ApplyBitwiseOp(*this, b, op);
}

Scalar Scalar::operator&(const Scalar &b) const
{
    auto op = [](uint8_t a, uint8_t b) -> uint8_t { return a & b; };
    return ApplyBitwiseOp(*this, b, op);
}

Scalar Scalar::operator~() const
{
    // Getting complement of lower 8 bytes only since when 32-byte buffer is fully complemented,
    // mclBrFr_deserialize returns undesired result
    int64_t n_complement_scalar = ~GetInt64();
    Scalar ret(n_complement_scalar);

    return ret;
}

Scalar Scalar::operator<<(unsigned int shift) const
{
    mclBnFr next;
    mclBnFr prev = m_fr;
    for (size_t i = 0; i < shift; ++i) {
        mclBnFr_add(&next, &prev, &prev);
        prev = next;
    }
    Scalar ret(prev);

    return ret;
}

/**
 * Assumes that fr contains a number within int64_t range
 */
Scalar Scalar::operator>>(unsigned int shift) const
{
    int64_t n = GetInt64();
    Scalar ret(n >> shift);

    return ret;
}

void Scalar::operator=(const uint64_t& n)
{
    mclBnFr_setInt(&m_fr, (mclInt)n);
}

bool Scalar::operator==(const int &b) const
{
    Scalar temp;
    temp = b;
    return mclBnFr_isEqual(&m_fr, &temp.m_fr);
}

bool Scalar::operator==(const Scalar &b) const
{
    return mclBnFr_isEqual(&m_fr, &b.m_fr);
}

bool Scalar::operator!=(const int &b) const
{
    return !operator==(b);
}

bool Scalar::operator!=(const Scalar &b) const
{
    return !operator==(b);
}

Scalar Scalar::Invert() const
{
    if (mclBnFr_isZero(&m_fr) == 1) {
        throw std::runtime_error("Inverse of zero is undefined");
    }
    Scalar temp;
    mclBnFr_inv(&temp.m_fr, &m_fr);
    return temp;
}

Scalar Scalar::Negate() const
{
    Scalar temp;
    mclBnFr_neg(&temp.m_fr, &m_fr);
    return temp;
}

Scalar Scalar::Square() const
{
    Scalar temp;
    mclBnFr_sqr(&temp.m_fr, &m_fr);
    return temp;
}

Scalar Scalar::Cube() const
{
    Scalar temp(m_fr);
    temp = temp * temp.Square();
    return temp;
}

Scalar Scalar::Pow(const Scalar& n) const
{
    // A variant of double-and-add method
    Scalar temp(1);
    mclBnFr bit_val;
    bit_val = m_fr;
    auto bits = n.GetBits();

    for (auto it = bits.rbegin(); it != bits.rend(); ++it) {
        Scalar s(bit_val);
        if (*it) {
            mclBnFr_mul(&temp.m_fr, &temp.m_fr, &bit_val);
        }
        mclBnFr_mul(&bit_val, &bit_val, &bit_val);
    }
    return temp;
}

Scalar Scalar::Rand(bool exclude_zero)
{
    Scalar temp;

    while (true) {
        if (mclBnFr_setByCSPRNG(&temp.m_fr) != 0) {
            throw std::runtime_error(std::string("Failed to generate random number"));
        }
        if (!exclude_zero || mclBnFr_isZero(&temp.m_fr) != 1) break;
    }
    return temp;
}

int64_t Scalar::GetInt64() const
{
    int64_t ret = 0;
    std::vector<uint8_t> vch = GetVch();
    for (auto i = 0; i < 8; ++i) {
        ret |= (int64_t) vch[vch.size()-i-1] << i*8;
    }
    return ret;
}

std::vector<uint8_t> Scalar::GetVch() const
{
    std::vector<uint8_t> b(WIDTH);
    if (mclBnFr_serialize(&b[0], WIDTH, &m_fr) == 0) {
        throw std::runtime_error(std::string("Failed to serialize mclBnFr"));
    }
    return b;
}

void Scalar::SetVch(const std::vector<uint8_t> &v)
{
    if (mclBnFr_setBigEndianMod(&m_fr, &v[0], v.size()) == -1) {
        throw std::runtime_error(std::string("Failed to setBigEndianMod vector"));
    }
}

void Scalar::SetPow2(int n)
{
    Scalar temp = 1;
    while (n != 0) {
        temp = temp * 2;
        --n;
    }

    m_fr = temp.m_fr;
}

uint256 Scalar::Hash(const int& n) const
{
    CHashWriter hasher(0,0);
    hasher << *this;
    hasher << n;
    return hasher.GetHash();
}

std::string Scalar::GetString(const int8_t radix) const
{
    char str[1024];

    if (mclBnFr_getStr(str, sizeof(str), &m_fr, radix) == 0) {
        throw std::runtime_error(std::string("Failed to get string representation of mclBnFr"));
    }
    return std::string(str);
}

std::vector<bool> Scalar::GetBits() const
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
bool Scalar::GetBit(uint8_t n) const
{
    std::vector<uint8_t> vch = GetVch();
    assert(vch.size() == 32);

    const uint8_t vchIdx = 31 - n / 8;  // vch is little-endian
    const uint8_t bitIdx = n % 8;
    const uint8_t mask = 1 << bitIdx;
    const bool bit = (vch[vchIdx] & mask) != 0;

    return bit;
}

unsigned int Scalar::GetSerializeSize() const
{
    return ::GetSerializeSize(GetVch());
}
