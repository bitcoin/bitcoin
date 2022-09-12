// Copyright (c) 2020 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "scalar.h"
#include <utilstrencodings.h>

#include <iostream>

void fp_inv_exgcd_bn(bn_t c, const bn_t u_, const bn_t p) {
    bn_t v, g1, g2, q, r, u;

    bn_null(v);
    bn_null(g1);
    bn_null(g2);
    bn_null(q);
    bn_null(r);

    RLC_TRY {
        bn_new(v);
        bn_new(g1);
        bn_new(g2);
        bn_new(q);
        bn_new(r);
        bn_new(u);

        /* u = a, v = p, g1 = 1, g2 = 0. */
        bn_copy(v, p);
        bn_copy(u, u_);
        bn_set_dig(g1, 1);
        bn_zero(g2);

        /* While (u != 1. */
        while (bn_cmp_dig(u, 1) != RLC_EQ) {
            /* q = [v/u], r = v mod u. */
            bn_div_rem(q, r, v, u);
            /* v = u, u = r. */
            bn_copy(v, u);
            bn_copy(u, r);
            /* r = g2 - q * g1. */
            bn_mul(r, q, g1);
            bn_sub(r, g2, r);
            /* g2 = g1, g1 = r. */
            bn_copy(g2, g1);
            bn_copy(g1, r);
        }

        if (bn_sign(g1) == RLC_NEG) {
            bn_add(g1, g1, p);
        }
        bn_copy(c, g1);
    }
    RLC_CATCH_ANY {
        RLC_THROW(ERR_CAUGHT);
    }
    RLC_FINALLY {
        bn_free(v);
        bn_free(g1);
        bn_free(g2);
        bn_free(q);
        bn_free(r);
    };
}

Scalar::Scalar()
{
    bn_new(bn);
    bn_zero(bn);
}

Scalar::Scalar(const std::vector<uint8_t> &v)
{
    bn_new(bn);
    bn_zero(bn);
    SetVch(v);
}

Scalar Scalar::operator+(const Scalar &b) const
{
    Scalar ret;
    bn_add(ret.bn, this->bn, b.bn);
    bn_t ord;
    bn_new(ord);
    g1_get_ord(ord);
    bn_mod(ret.bn, ret.bn, ord);
    return ret;
}

Scalar Scalar::operator-(const Scalar &b) const
{
    Scalar ret;
    bn_sub(ret.bn, this->bn, b.bn);
    bn_t ord;
    bn_new(ord);
    g1_get_ord(ord);
    bn_mod(ret.bn, ret.bn, ord);
    return ret;
}

Scalar Scalar::operator*(const Scalar &b) const
{
    Scalar ret;
    bn_mul(ret.bn, this->bn, b.bn);
    bn_t ord;
    bn_new(ord);
    g1_get_ord(ord);
    bn_mod(ret.bn, ret.bn, ord);
    return ret;
}

Scalar Scalar::operator<<(const int &b) const
{
    Scalar ret;
    bn_lsh(ret.bn, this->bn, b);
    return ret;
}

Scalar Scalar::operator>>(const int &b) const
{
    Scalar ret;
    bn_rsh(ret.bn, this->bn, b);
    return ret;
}

Scalar Scalar::operator|(const Scalar &b) const
{
    Scalar ret;
    size_t size = std::max(bn_bits(this->bn), bn_bits(b.bn));

    for (size_t i = 0; i < size; i++)
    {
        bool l = bn_get_bit(this->bn, i);
        bool r = bn_get_bit(b.bn, i);
        bn_set_bit(ret.bn, i, l|r);
    }

    return ret;
}

Scalar Scalar::operator^(const Scalar &b) const
{
    Scalar ret;

    size_t size = std::max(bn_bits(this->bn), bn_bits(b.bn));

    for (size_t i = 0; i < size; i++)
    {
        bool l = bn_get_bit(this->bn, i);
        bool r = bn_get_bit(b.bn, i);
        bn_set_bit(ret.bn, i, l^r);
    }

    return ret;
}

Scalar Scalar::operator&(const Scalar &b) const
{
    Scalar ret;
    size_t size = std::max(bn_bits(this->bn), bn_bits(b.bn));

    for (size_t i = 0; i < size; i++)
    {
        bool l = bn_get_bit(this->bn, i);
        bool r = bn_get_bit(b.bn, i);
        bn_set_bit(ret.bn, i, l&r);
    }

    return ret;
}

Scalar Scalar::operator~() const
{
    Scalar ret;
    size_t size = bn_bits(this->bn);

    for (size_t i = 0; i < size; i++)
    {
        bool a = bn_get_bit(this->bn, i);
        bn_set_bit(ret.bn, i, !a);
    }

    return ret;
}

void Scalar::operator=(const uint64_t& n)
{
    bn_new(this->bn);
    bn_zero(this->bn);
    for (size_t i = 0; i < 64; i++)
    {
        bn_set_bit(this->bn, i, (n>>i)&1);
    }
}

Scalar::Scalar(const uint64_t& n)
{
    bn_new(this->bn);
    bn_zero(this->bn);
    for (size_t i = 0; i < 64; i++)
    {
        bn_set_bit(this->bn, i, (n>>i)&1);
    }
}

Scalar::Scalar(const Scalar& n)
{
    bn_new(this->bn);
    bn_zero(this->bn);
    bn_copy(this->bn, n.bn);
    bn_t ord;
    bn_new(ord);
    g1_get_ord(ord);
    bn_mod(this->bn, this->bn, ord);
}

Scalar::Scalar(const bls::PrivateKey& n)
{
    bn_new(this->bn);
    bn_zero(this->bn);
    uint8_t buf[bls::PrivateKey::PRIVATE_KEY_SIZE];
    n.Serialize(buf);
    bn_read_bin(this->bn, (unsigned char*)&buf, 32);
    bn_t ord;
    bn_new(ord);
    g1_get_ord(ord);
    bn_mod(this->bn, this->bn, ord);
}

bool Scalar::operator==(const int &b) const
{
    Scalar temp;
    temp = b;
    return bn_cmp(this->bn, temp.bn) == RLC_EQ;
}

bool Scalar::operator==(const Scalar &b) const
{
    return bn_cmp(this->bn, b.bn) == RLC_EQ;
}

std::vector<uint8_t> Scalar::GetVch() const
{
    std::vector<uint8_t> ret(bls::PrivateKey::PRIVATE_KEY_SIZE);
    bn_write_bin(ret.data(), bls::PrivateKey::PRIVATE_KEY_SIZE, this->bn);
    return ret;
}

Scalar Scalar::Invert() const
{
    bn_t ord;
    bn_new(ord);
    g1_get_ord(ord);
    Scalar inv;
    Scalar toinverse;
    toinverse = *this;
    fp_inv_exgcd_bn(inv.bn, toinverse.bn, ord);

    bn_mod(inv.bn, inv.bn, ord);

    CHECK_AND_ASSERT_THROW_MES((*this * inv) == 1, "Invert failed");

    return inv;
}

int64_t Scalar::GetInt64() const
{
    int64_t ret = 0;
    for (int64_t i = 0; i < 64; i++)
    {
        ret |= (int64_t)bn_get_bit(this->bn, i) << i;
    }
    return ret;
}

bool Scalar::GetBit(size_t n) const
{
    return bn_get_bit(this->bn, n);
}

Scalar Scalar::Rand()
{
    bn_t ord;
    bn_new(ord);
    g1_get_ord(ord);
    bn_t ret;
    bn_rand_mod(ret, ord);
    Scalar r;
    r = ret;
    return r;
}

uint256 Scalar::Hash(const int& n) const
{
    CHashWriter hasher(0,0);
    hasher << *this;
    hasher << n;
    return hasher.GetHash();
}

Scalar::Scalar(const bn_t& n)
{
    bn_new(this->bn);
    bn_zero(this->bn);
    bn_copy(this->bn, n);
    bn_t ord;
    bn_new(ord);
    g1_get_ord(ord);
    bn_mod(this->bn, this->bn, ord);
}

Scalar::Scalar(const uint256 &b)
{
    bn_new(this->bn);
    bn_zero(this->bn);
    bn_read_bin(this->bn, (unsigned char*)&b, 32);
    bn_t ord;
    bn_new(ord);
    g1_get_ord(ord);
    bn_mod(this->bn, this->bn, ord);
}

void Scalar::SetVch(const std::vector<uint8_t> &b)
{
    bn_read_bin(this->bn, &b.front(), b.size());
}

Scalar Scalar::Negate() const
{
    Scalar ret;
    bn_neg(ret.bn, this->bn);
    bn_t ord;
    bn_new(ord);
    g1_get_ord(ord);
    bn_mod(ret.bn, ret.bn, ord);
    return ret;
}

void Scalar::SetPow2(const int& n)
{
    bn_set_2b(this->bn, n);
}

uint256 HashG1Element(bls::G1Element g1, uint64_t n)
{
    CHashWriter hasher(0,0);
    hasher << g1.Serialize();
    hasher << n;
    return hasher.GetHash();
}
