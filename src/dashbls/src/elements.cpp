// Copyright 2020 Chia Network Inc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstring>

#include "bls.hpp"
#include "legacy.hpp"

namespace bls {

const size_t G1Element::SIZE;

G1Element G1Element::FromBytes(Bytes const bytes, bool fLegacy) {
    G1Element ele = G1Element::FromBytesUnchecked(bytes, fLegacy);
    if (!fLegacy) {
        ele.CheckValid();
    }
    return ele;
}

G1Element G1Element::FromBytesUnchecked(Bytes const bytes, bool fLegacy)
{
    if (bytes.size() != SIZE) {
        throw std::invalid_argument("G1Element::FromBytes: Invalid size");
    }

    G1Element ele;

    // convert bytes to relic form
    uint8_t buffer[G1Element::SIZE + 1];
    std::memcpy(buffer + 1, bytes.begin(), G1Element::SIZE);
    buffer[0] = 0x00;
    buffer[1] &= 0x1f;  // erase 3 msbs from given input

    if ((bytes[0] & 0xc0) == 0xc0) {  // representing infinity
        buffer[0] = 0x00;
        buffer[1] &= 0x1f;  // erase 3 msbs from given input

        bool fZerosOnly = Util::HasOnlyZeros(Bytes(buffer, G1Element::SIZE + 1));
        // enforce that infinity must be 0xc0000..00
        if (bytes[0] != 0xc0 || !fZerosOnly) {
            throw std::invalid_argument("Given G1 infinity element must be canonical");
        }
        return ele;
    } else {
        if (fLegacy) {
            if (bytes[0] & 0x80) {
                buffer[0] = 0x03;   // Insert extra byte for Y=1
                buffer[1] &= 0x7f;  // Remove initial Y bit
            } else {
                buffer[0] = 0x02;   // Insert extra byte for Y=0
            }
        } else {
            buffer[0] = 0x00;
            buffer[1] &= 0x1f;  // erase 3 msbs from given input

            bool fZerosOnly = Util::HasOnlyZeros(Bytes(buffer, G1Element::SIZE + 1));
            if ((bytes[0] & 0xc0) != 0x80) {
                throw std::invalid_argument(
                        "Given G1 non-infinity element must start with 0b10");
            }

            if (fZerosOnly) {
                throw std::invalid_argument("G1 non-infinity element can't have only zeros");
            }
            if (bytes[0] & 0x20) {  // sign bit
                buffer[0] = 0x03;
            } else {
                buffer[0] = 0x02;
            }
        }
    }
    g1_read_bin(ele.p, buffer, G1Element::SIZE + 1);
    BLS::CheckRelicErrors(!fLegacy);
    return ele;
}

G1Element G1Element::FromByteVector(const std::vector<uint8_t>& bytevec, bool fLegacy)
{
    return G1Element::FromBytes(Bytes(bytevec), fLegacy);
}

G1Element G1Element::FromNative(const g1_t element)
{
    G1Element ele;
    g1_copy(ele.p, element);
    return ele;
}

G1Element G1Element::Copy() {
    G1Element ele;
    g1_copy(ele.p, this->p);

    return ele;
}


G1Element G1Element::FromMessage(const std::vector<uint8_t>& message,
                                 const uint8_t* dst,
                                 int dst_len)
{
    return FromMessage(Bytes(message), dst, dst_len);
}

G1Element G1Element::FromMessage(Bytes const message,
                                 const uint8_t* dst,
                                 int dst_len)
{
    G1Element ans;
    ep_map_dst(ans.p, message.begin(), (int)message.size(), dst, dst_len);
    BLS::CheckRelicErrors();
    assert(ans.IsValid());
    return ans;
}

G1Element G1Element::Generator()
{
    G1Element ele;
    g1_get_gen(ele.p);
    BLS::CheckRelicErrors();
    return ele;
}

bool G1Element::IsValid() const {
    // Infinity no longer valid in Relic
    // https://github.com/relic-toolkit/relic/commit/f3be2babb955cf9f82743e0ae5ef265d3da6c02b
    if (g1_is_infty((g1_st*)p) == 1)
        return true;

    return g1_is_valid((g1_st*)p);
}

void G1Element::CheckValid() const {
    if (!IsValid())
        throw std::invalid_argument("G1 element is invalid");
    BLS::CheckRelicErrors();
}

void G1Element::ToNative(g1_t output) const {
    g1_copy(output, p);
}

G1Element G1Element::Negate() const
{
    G1Element ans;
    g1_neg(ans.p, p);
    BLS::CheckRelicErrors();
    return ans;
}

GTElement G1Element::Pair(const G2Element& b) const { return (*this) & b; }

uint32_t G1Element::GetFingerprint(const bool fLegacy) const
{
    uint8_t buffer[G1Element::SIZE];
    uint8_t hash[32];
    memcpy(buffer, Serialize(fLegacy).data(), G1Element::SIZE);
    Util::Hash256(hash, buffer, G1Element::SIZE);
    return Util::FourBytesToInt(hash);
}

std::vector<uint8_t> G1Element::Serialize(const bool fLegacy) const {
    uint8_t buffer[G1Element::SIZE + 1];
    g1_write_bin(buffer, G1Element::SIZE + 1, p, 1);

    if (buffer[0] == 0x00) {  // infinity
        std::vector<uint8_t> result(G1Element::SIZE, 0);
        result[0] = 0xc0;
        return result;
    }

    if (buffer[0] == 0x03) {  // sign bit set
        buffer[1] |= fLegacy ? 0x80 : 0x20;
    }

    if (!fLegacy) {
        buffer[1] |= 0x80;  // indicate compression
    }
    return std::vector<uint8_t>(buffer + 1, buffer + 1 + G1Element::SIZE);
}

bool operator==(const G1Element & a, const G1Element &b)
{
    return g1_cmp(a.p, b.p) == RLC_EQ;
}

bool operator!=(const G1Element & a, const G1Element & b) { return !(a == b); }

std::ostream& operator<<(std::ostream& os, const G1Element &ele)
{
    return os << Util::HexStr(ele.Serialize());
}

G1Element& operator+=(G1Element& a, const G1Element& b)
{
    g1_add(a.p, a.p, b.p);
    BLS::CheckRelicErrors();
    return a;
}

G1Element operator+(const G1Element& a, const G1Element& b)
{
    G1Element ans;
    g1_add(ans.p, a.p, b.p);
    BLS::CheckRelicErrors();
    return ans;
}

G1Element operator*(const G1Element& a, const bn_t& k)
{
    G1Element ans;
    g1_mul(ans.p, (g1_st*)a.p, (bn_st*)k);
    BLS::CheckRelicErrors();
    return ans;
}

G1Element operator*(const bn_t& k, const G1Element& a) { return a * k; }



// G2Element definitions below



const size_t G2Element::SIZE;

G2Element G2Element::FromBytes(Bytes const bytes, const bool fLegacy) {
    G2Element ele = G2Element::FromBytesUnchecked(bytes, fLegacy);
    if (!fLegacy) {
        ele.CheckValid();
    }
    return ele;
}

G2Element G2Element::FromBytesUnchecked(Bytes const bytes, const bool fLegacy)
{
    if (bytes.size() != SIZE) {
        throw std::invalid_argument("G2Element::FromBytes: Invalid size");
    }

    G2Element ele;
    uint8_t buffer[G2Element::SIZE + 1];

    if (fLegacy) {
        std::memcpy(buffer + 1, bytes.begin(), G2Element::SIZE);
        buffer[0] = 0x00;
    } else {
        std::memcpy(buffer + 1, bytes.begin() + G2Element::SIZE / 2, G2Element::SIZE / 2);
        std::memcpy(buffer + 1 + G2Element::SIZE / 2, bytes.begin(), G2Element::SIZE / 2);
        buffer[0] = 0x00;
        buffer[49] &= 0x1f;  // erase 3 msbs from input
        if ((bytes[48] & 0xe0) != 0x00) {
            throw std::invalid_argument(
                    "Given G2 element must always have 48th byte start with 0b000");
        }
    }
    bool fZerosOnly = Util::HasOnlyZeros(Bytes(buffer, G2Element::SIZE + 1));
    if (((bytes[0] & 0xc0) == 0xc0)) {  // infinity
        // enforce that infinity must be 0xc0000..00
        if (bytes[0] != 0xc0 || !fZerosOnly) {
            throw std::invalid_argument(
                "Given G2 infinity element must be canonical");
        }
        return ele;
    }

    if (fLegacy) {
        if (bytes[0] & 0x80) {
            buffer[0] = 0x03;   // Insert extra byte for Y=1
            buffer[1] &= 0x7f;  // Remove initial Y bit
        } else {
            buffer[0] = 0x02;   // Insert extra byte for Y=0
        }
    } else {
        if (((bytes[0] & 0xc0) != 0x80)) {
            throw std::invalid_argument(
                "G2 non-inf element must have 0th byte start with 0b10");
        }

        if (fZerosOnly) {
            throw std::invalid_argument("G2 non-infinity element can't have only zeros");
        }

        if (bytes[0] & 0x20) {
            buffer[0] = 0x03;
        } else {
            buffer[0] = 0x02;
        }
    }

    g2_read_bin(ele.q, buffer, G2Element::SIZE + 1);
    BLS::CheckRelicErrors(!fLegacy);
    return ele;
}

G2Element G2Element::FromByteVector(const std::vector<uint8_t>& bytevec, bool fLegacy)
{
    return G2Element::FromBytes(Bytes(bytevec), fLegacy);
}

G2Element G2Element::FromNative(const g2_t element)
{
    G2Element ele;
    g2_copy(ele.q, (g2_st*)element);
    return ele;
}

G2Element G2Element::FromMessage(const std::vector<uint8_t>& message,
                                 const uint8_t* dst,
                                 int dst_len,
                                 const bool fLegacy)
{
    return FromMessage(Bytes(message), dst, dst_len, fLegacy);
}

G2Element G2Element::FromMessage(Bytes const message,
                                 const uint8_t* dst,
                                 int dst_len,
                                 const bool fLegacy)
{
    G2Element ans;
    if (fLegacy) {
        ep2_map_legacy(ans.q, message.begin(), BLS::MESSAGE_HASH_LEN);
    } else {
        ep2_map_dst(ans.q, message.begin(), (int)message.size(), dst, dst_len);
    }
    BLS::CheckRelicErrors();
    assert(ans.IsValid());
    return ans;
}

G2Element G2Element::Generator()
{
    G2Element ele;
    g2_get_gen(ele.q);
    BLS::CheckRelicErrors();
    return ele;
}

bool G2Element::IsValid() const {
    // Infinity no longer valid in Relic
    // https://github.com/relic-toolkit/relic/commit/f3be2babb955cf9f82743e0ae5ef265d3da6c02b
    if (g2_is_infty((g2_st*)q) == 1)
        return true;

    return g2_is_valid((g2_st*)q);
}

void G2Element::CheckValid() const {
    if (!IsValid())
        throw std::invalid_argument("G2 element is invalid");
    BLS::CheckRelicErrors();
}

void G2Element::ToNative(g2_t output) const {
    g2_copy(output, (g2_st*)q);
}

G2Element G2Element::Copy() {
    G2Element ele;
    g2_copy(ele.q, this->q);

    return ele;
}


G2Element G2Element::Negate() const
{
    G2Element ans;
    g2_neg(ans.q, (g2_st*)q);
    BLS::CheckRelicErrors();
    return ans;
}

GTElement G2Element::Pair(const G1Element& a) const { return a & (*this); }

std::vector<uint8_t> G2Element::Serialize(const bool fLegacy) const {
    uint8_t buffer[G2Element::SIZE + 1];
    g2_write_bin(buffer, G2Element::SIZE + 1, (g2_st*)q, 1);

    if (buffer[0] == 0x00) {  // infinity
        std::vector<uint8_t> result(G2Element::SIZE, 0);
        result[0] = 0xc0;
        return result;
    }

    if (fLegacy) {
        if (buffer[0] == 0x03) {  // sign bit set
            buffer[1] |= 0x80;
        }
    } else {
        // remove leading 3 bits
        buffer[1] &= 0x1f;
        buffer[49] &= 0x1f;
        if (buffer[0] == 0x03) {
            buffer[49] |= 0xa0;  // swapped later to 0
        } else {
            buffer[49] |= 0x80;
        }
    }

    std::vector<uint8_t> result(G2Element::SIZE, 0);
    if (fLegacy) {
        std::memcpy(result.data(), buffer + 1, G2Element::SIZE);
    } else {
        // Swap buffer, relic uses the opposite ordering for Fq2 elements
        std::memcpy(result.data(), buffer + 1 + G2Element::SIZE / 2, G2Element::SIZE / 2);
        std::memcpy(result.data() + G2Element::SIZE / 2, buffer + 1, G2Element::SIZE / 2);
    }

    return result;
}

bool operator==(G2Element const& a, G2Element const& b)
{
    return g2_cmp((g2_st*)a.q, (g2_st*)b.q) == RLC_EQ;
}

bool operator!=(G2Element const& a, G2Element const& b) { return !(a == b); }

std::ostream& operator<<(std::ostream& os, const G2Element & s)
{
    return os << Util::HexStr(s.Serialize());
}

G2Element& operator+=(G2Element& a, const G2Element& b)
{
    g2_add(a.q, a.q, (g2_st*)b.q);
    BLS::CheckRelicErrors();
    return a;
}

G2Element operator+(const G2Element& a, const G2Element& b)
{
    G2Element ans;
    g2_add(ans.q, (g2_st*)a.q, (g2_st*)b.q);
    BLS::CheckRelicErrors();
    return ans;
}

G2Element operator*(const G2Element& a, const bn_t& k)
{
    G2Element ans;
    g2_mul(ans.q, (g2_st*)a.q, (bn_st*)k);
    BLS::CheckRelicErrors();
    return ans;
}

G2Element operator*(const bn_t& k, const G2Element& a) { return a * k; }



// GTElement

const size_t GTElement::SIZE;

GTElement GTElement::FromBytes(Bytes const bytes)
{
    GTElement ele = GTElement::FromBytesUnchecked(bytes);
    if (gt_is_valid(*(gt_t*)&ele) == 0)
        throw std::invalid_argument("GTElement is invalid");
    return ele;
}

GTElement GTElement::FromBytesUnchecked(Bytes const bytes)
{
    if (bytes.size() != SIZE) {
        throw std::invalid_argument("GTElement::FromBytes: Invalid size");
    }
    GTElement ele = GTElement();
    gt_read_bin(ele.r, bytes.begin(), GTElement::SIZE);
    BLS::CheckRelicErrors();
    return ele;
}

GTElement GTElement::FromByteVector(const std::vector<uint8_t>& bytevec)
{
    return GTElement::FromBytes(Bytes(bytevec));
}

GTElement GTElement::FromNative(const gt_t* element)
{
    GTElement ele = GTElement();
    gt_copy(ele.r, *(gt_t*)element);
    return ele;
}

GTElement GTElement::Unity() {
    GTElement ele = GTElement();
    gt_set_unity(ele.r);
    return ele;
}


bool operator==(GTElement const& a, GTElement const& b)
{
    return gt_cmp(*(gt_t*)(a.r), *(gt_t*)(b.r)) == RLC_EQ;
}

bool operator!=(GTElement const& a, GTElement const& b) { return !(a == b); }

std::ostream& operator<<(std::ostream& os, GTElement const& ele)
{
    return os << Util::HexStr(ele.Serialize());
}

GTElement operator&(const G1Element& a, const G2Element& b)
{
    G1Element nonConstA(a);
    gt_t ans;
    gt_new(ans);
    g2_t tmp;
    g2_null(tmp);
    g2_new(tmp);
    b.ToNative(tmp);
    pp_map_oatep_k12(ans, nonConstA.p, tmp);
    GTElement ret = GTElement::FromNative(&ans);
    gt_free(ans);
    g2_free(tmp);
    return ret;
}

GTElement operator*(GTElement& a, GTElement& b)
{
    GTElement ans;
    fp12_mul(ans.r, a.r, b.r);
    BLS::CheckRelicErrors();
    return ans;
}

void GTElement::Serialize(uint8_t* buffer) const
{
    gt_write_bin(buffer, GTElement::SIZE, *(gt_t*)&r, 1);
}

std::vector<uint8_t> GTElement::Serialize() const
{
    std::vector<uint8_t> data(GTElement::SIZE);
    Serialize(data.data());
    return data;
}

}  // end namespace bls
