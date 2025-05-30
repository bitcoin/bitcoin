// Copyright (c) 2017, 2021 Pieter Wuille
// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bech32.h>
#include <codex32.h>
#include <util/vector.h>

#include <array>
#include <assert.h>
#include <numeric>
#include <optional>

namespace codex32
{

namespace
{

typedef bech32::internal::data data;

// Build multiplication and logarithm tables for GF(32).
//
// We represent GF(32) as an extension of GF(2) by appending a root, alpha, of the
// polynomial x^5 + x^3 + 1. All elements of GF(32) can be represented as degree-4
// polynomials in alpha. So e.g. 1 is represented by 1, alpha by 2, alpha^2 by 4,
// and so on.
//
// alpha is also a generator of the multiplicative group of the field. So every nonzero
// element in GF(32) can be represented as alpha^i, for some i in {0, 1, ..., 31}.
// This representation makes multiplication and division very easy, since it is just
// addition and subtraction in the exponent.
//
// These tables allow converting from the normal binary representation of GF(32) elements
// to the power-of-alpha one.
constexpr std::pair<std::array<int8_t, 31>, std::array<int8_t, 32>> GenerateGF32Tables() {
    // We use these tables to perform arithmetic in GF(32) below, when constructing the
    // tables for GF(1024).
    std::array<int8_t, 31> GF32_EXP{};
    std::array<int8_t, 32> GF32_LOG{};

    // fmod encodes the defining polynomial of GF(32) over GF(2), x^5 + x^3 + 1.
    // Because coefficients in GF(2) are binary digits, the coefficients are packed as 101001.
    const int fmod = 41;

    // Elements of GF(32) are encoded as vectors of length 5 over GF(2), that is,
    // 5 binary digits. Each element (b_4, b_3, b_2, b_1, b_0) encodes a polynomial
    // b_4*x^4 + b_3*x^3 + b_2*x^2 + b_1*x^1 + b_0 (modulo fmod).
    // For example, 00001 = 1 is the multiplicative identity.
    GF32_EXP[0] = 1;
    GF32_LOG[0] = -1;
    GF32_LOG[1] = 0;
    int v = 1;
    for (int i = 1; i < 31; ++i) {
        // Multiplication by x is the same as shifting left by 1, as
        // every coefficient of the polynomial is moved up one place.
        v = v << 1;
        // If the polynomial now has an x^5 term, we subtract fmod from it
        // to remain working modulo fmod. Subtraction is the same as XOR in characteristic
        // 2 fields.
        if (v & 32) v ^= fmod;
        GF32_EXP[i] = v;
        GF32_LOG[v] = i;
    }

    return std::make_pair(GF32_EXP, GF32_LOG);
}

constexpr auto tables32 = GenerateGF32Tables();
constexpr const std::array<int8_t, 31>& GF32_EXP = tables32.first;
constexpr const std::array<int8_t, 32>& GF32_LOG = tables32.second;

uint8_t gf32_mul(uint8_t x, uint8_t y) {
    if (x == 0 || y == 0) {
        return 0;
    }
    return GF32_EXP[(GF32_LOG[x] + GF32_LOG[y]) % 31];
}

uint8_t gf32_div(uint8_t x, uint8_t y) {
    assert(y != 0);
    if (x == 0) {
        return 0;
    }
    return GF32_EXP[(GF32_LOG[x] + 31 - GF32_LOG[y]) % 31];
}

// The bech32 string "secretshare32"
constexpr const std::array<uint8_t, 13> CODEX32_M = {
    16, 25, 24, 3, 25, 11, 16, 23, 29, 3, 25, 17, 10
};

// The bech32 string "secretshare32ex"
constexpr const std::array<uint8_t, 15> CODEX32_LONG_M = {
    16, 25, 24, 3, 25, 11, 16, 23, 29, 3, 25, 17, 10, 25, 6,
};

// The generator for the codex32 checksum, not including the leading x^13 term
constexpr const std::array<uint8_t, 13> CODEX32_GEN = {
    25, 27, 17, 8, 0, 25, 25, 25, 31, 27, 24, 16, 16,
};

// The generator for the long codex32 checksum, not including the leading x^15 term
constexpr const std::array<uint8_t, 15> CODEX32_LONG_GEN = {
    15, 10, 25, 26, 9, 25, 21, 6, 23, 21, 6, 5, 22, 4, 23,
};

/** This function will compute what 5-bit values to XOR into the last <checksum length>
 *  input values, in order to make the checksum 0. These values are returned in an array
 *  whose length is implied by the type of the generator polynomial (`CODEX32_GEN` or
 *  `CODEX32_LONG_GEN`) that is passed in. The result should be xored with the target
 *  residue ("secretshare32" or "secretshare32ex". */
template <typename Residue>
Residue PolyMod(const data& v, const Residue& gen)
{
    // The input is interpreted as a list of coefficients of a polynomial over F = GF(32),
    // in the same way as in bech32. The block comment in bech32::<anonymous>::PolyMod
    // provides more details.
    //
    // Unlike bech32, the output consists of 13 5-bit values, rather than 6, so they cannot
    // be packed into a uint32_t, or even a uint64_t.
    //
    // Like bech32 we have a generator polynomial which defines the BCH code. For "short"
    // strings, whose data part is 93 characters or less, we use
    //     g(x) = x^13 + {25}x^12 + {27}x^11 + {17}x^10 + {8}x^9 + {0}x^8 + {25}x^7
    //               + {25}x^6  + {25}x^5 + {31}x^4 + {27}x^3 + {24}x^2 + {16}x + {16}
    //
    // For long strings, whose data part is more than 93 characters, we use
    //     g(x) = x^15 + {15}x^14 + {10}x^13 + {25}x^12 + {26}x^11 + {9}x^10
    //               + {25}x^9 + {21}x^8 + {6}x^7 + {23}x^6 + {21}x^5 + {6}x^4
    //               + {5}x^3  + {22}x^2 + {4}x^1 + {23}
    //
    // In both cases g is chosen in such a way that the resulting code is a BCH code which
    // can detect up to 8 errors in a window of 93 characters. Unlike bech32, no further
    // optimization was done to achieve more detection capability than the design parameters.
    //
    // For information about the {n} encoding of GF32 elements, see the block comment in
    // bech32::<anonymous>::PolyMod.
    Residue res{};
    res[gen.size() - 1] = 1;
    for (const auto v_i : v) {
        // We want to update `res` to correspond to a polynomial with one extra term. That is,
        // we first multiply it by x and add the next character, which is done by left-shifting
        // the entire array and adding the next character to the open slot.
        //
        // We then reduce it module g, which involves taking the shifted-off character, multiplying
        // it by g, and adding it to the result of the previous step. This makes sense because after
        // multiplying by x, `res` has the same degree as g, so reduction by g simply requires
        // dividing the most significant coefficient of `res` by the most significant coefficient of
        // g (which is 1), then subtracting that multiple of g.
        //
        // Recall that we are working in a characteristic-2 field, so that subtraction is the same
        // thing as addition.

        // Multiply by x
        uint8_t shift = res[0];
        for (size_t i = 1; i < res.size(); ++i) {
            res[i - 1] = res[i];
        }
        // Add the next value
        res[res.size() - 1] = v_i;
        // Reduce
        if (shift != 0) {
            for(size_t i = 0; i < res.size(); ++i) {
                if (gen[i] != 0) {
                    res[i] ^= gf32_mul(gen[i], shift);
                }
            }
        }
    }
    return res;
}

/** Verify a checksum. */
template <typename Residue>
bool VerifyChecksum(const std::string& hrp, const data& values, const Residue& gen, const Residue& target)
{
    auto enc = bech32::internal::PreparePolynomialCoefficients(hrp, values, 0);
    auto res = PolyMod(enc, gen);
    for (size_t i = 0; i < res.size(); ++i) {
        if (res[i] != target[i]) {
            return 0;
        }
    }
    return 1;
}

/** Create a checksum. */
template <typename Residue>
data CreateChecksum(const std::string& hrp, const data& values, const Residue& gen, const Residue& target)
{
    auto enc = bech32::internal::PreparePolynomialCoefficients(hrp, values, gen.size());
    enc.insert(enc.end(), gen.size(), 0x00);
    const auto checksum = PolyMod(enc, gen);
    data ret(gen.size());
    for (size_t i = 0; i < checksum.size(); ++i) {
        ret[i] = checksum[i] ^ target[i];
    }
    return ret;
}

// Given a set of share indices and a target index `idx`, which must be in the set,
// compute the Lagrange basis polynomial for `idx` evaluated at the point `eval`.
//
// All inputs are GF32 elements, rather than array indices or anything else.
uint8_t lagrange_coefficient(std::vector<uint8_t>& indices, uint8_t idx, uint8_t eval) {
    uint8_t num = 1;
    uint8_t den = 1;
    for (const auto idx_i : indices) {
        if (idx_i != idx) {
            num = gf32_mul(num, idx_i ^ eval);
            den = gf32_mul(den, idx_i ^ idx);
        }
    }

    // return num / den
    return gf32_div(num, den);
}

} // namespace

std::string ErrorString(Error e) {
    switch (e) {
    case OK: return "ok";
    case BAD_CHECKSUM: return "bad checksum";
    case BECH32_DECODE: return "bech32 decode failure (invalid character, no HRP, or inconsistent case)";
    case INVALID_HRP: return "hrp differed from 'ms'";
    case INVALID_ID_LEN: return "seed ID was not 4 characters";
    case INVALID_ID_CHAR: return "seed ID used a non-bech32 character";
    case INVALID_LENGTH: return "invalid length";
    case INVALID_K: return "invalid threshold (k) value";
    case INVALID_SHARE_IDX: return "invalid share index";
    case TOO_FEW_SHARES: return "tried to derive a share but did not have enough input shares";
    case DUPLICATE_SHARE: return "tried to derive a share but two input shares had the same index";
    case MISMATCH_K: return "tried to derive a share but input shares had inconsistent threshold (k) values";
    case MISMATCH_ID: return "tried to derive a share but input shares had inconsistent seed IDs";
    case MISMATCH_LENGTH: return "tried to derive a share but input shares had inconsistent lengths";
    }
    assert(0);
}

/** Encode a codex32 string. */
std::string Result::Encode() const {
    assert(IsValid());

    const data checksum = m_data.size() <= 80
        ? CreateChecksum(m_hrp, m_data, CODEX32_GEN, CODEX32_M)
        : CreateChecksum(m_hrp, m_data, CODEX32_LONG_GEN, CODEX32_LONG_M);
    return bech32::internal::Encode(m_hrp, m_data, checksum);
}

/** Decode a codex32 string */
Result::Result(const std::string& str) {
    m_valid = OK;

    auto res = bech32::internal::Decode(str, 127, 6);

    if (str.size() > 127) {
        m_valid = INVALID_LENGTH;
        // Early return since if we failed the max size check, Decode did not give us any data.
        return;
    } else if (res.first.empty() && res.second.empty()) {
        m_valid = BECH32_DECODE;
        return;
    } else if (res.first != "ms") {
        m_valid = INVALID_HRP;
        // Early return since if the HRP is wrong, all bets are off and no point continuing
        return;
    }
    m_hrp = std::move(res.first);

    if (res.second.size() >= 45 && res.second.size() <= 90) {
        // If, after converting back to base-256, we have 5 or more bits of data
        // remaining, it means that we had an entire character of useless data,
        // which shouldn't have been included.
        if (((res.second.size() - 6 - 13) * 5) % 8 > 4) {
            m_valid = INVALID_LENGTH;
        } else if (VerifyChecksum(m_hrp, res.second, CODEX32_GEN, CODEX32_M)) {
            m_data = data(res.second.begin(), res.second.end() - 13);
        } else {
            m_valid = BAD_CHECKSUM;
        }
    } else if (res.second.size() >= 96 && res.second.size() <= 124) {
        if (((res.second.size() - 6 - 15) * 5) % 8 > 4) {
            m_valid = INVALID_LENGTH;
        } else if (VerifyChecksum(m_hrp, res.second, CODEX32_LONG_GEN, CODEX32_LONG_M)) {
            m_data = data(res.second.begin(), res.second.end() - 15);
        } else {
            m_valid = BAD_CHECKSUM;
        }
    } else {
        m_valid = INVALID_LENGTH;
    }

    if (m_valid == OK) {
        auto k = bech32::internal::CHARSET[res.second[0]];
        if (k < '0' || k == '1' || k > '9') {
            m_valid = INVALID_K;
        }
        if (k == '0' && m_data[5] != 16) {
            // If the threshold is 0, the only allowable share is S
            m_valid = INVALID_SHARE_IDX;
        }
    }
}

Result::Result(std::string&& hrp, size_t k, const std::string& id, char share_idx, const std::vector<unsigned char>& data) {
    m_valid = OK;
    if (hrp != "ms") {
        m_valid = INVALID_HRP;
    }
    m_hrp = hrp;
    if (k == 1 || k > 9) {
        m_valid = INVALID_K;
    }
    if (id.size() != 4) {
        m_valid = INVALID_ID_LEN;
    }
    int8_t sidx = bech32::internal::CHARSET_REV[(unsigned char) share_idx];
    if (sidx == -1) {
        m_valid = INVALID_SHARE_IDX;
    }
    if (k == 0 && sidx != 16) {
        // If the threshold is 0, the only allowable share is S
        m_valid = INVALID_SHARE_IDX;
    }
    for (size_t i = 0; i < id.size(); ++i) {
        if (bech32::internal::CHARSET_REV[(unsigned char) id[i]] == -1) {
            m_valid = INVALID_ID_CHAR;
        }
    }

    if (m_valid != OK) {
        // early bail before allocating memory
        return;
    }

    m_data.reserve(6 + ((data.size() * 8) + 4) / 5);
    m_data.push_back(bech32::internal::CHARSET_REV['0' + k]);
    m_data.push_back(bech32::internal::CHARSET_REV[(unsigned char) id[0]]);
    m_data.push_back(bech32::internal::CHARSET_REV[(unsigned char) id[1]]);
    m_data.push_back(bech32::internal::CHARSET_REV[(unsigned char) id[2]]);
    m_data.push_back(bech32::internal::CHARSET_REV[(unsigned char) id[3]]);
    m_data.push_back(sidx);
    ConvertBits<8, 5, true>([&](unsigned char c) { m_data.push_back(c); }, data.begin(), data.end());
}

Result::Result(const std::vector<Result>& shares, char output_idx) {
    m_valid = OK;

    int8_t oidx = bech32::internal::CHARSET_REV[(unsigned char) output_idx];
    if (oidx == -1) {
        m_valid = INVALID_SHARE_IDX;
    }
    if (shares.empty()) {
        m_valid = TOO_FEW_SHARES;
        return;
    }
    size_t k = shares[0].GetK();
    if (k > shares.size()) {
        m_valid = TOO_FEW_SHARES;
    }
    if (m_valid != OK) {
        return;
    }

    std::vector<uint8_t> indices;
    indices.reserve(shares.size());
    for (size_t i = 0; i < shares.size(); ++i) {
        // Currently the only supported hrp is "ms" so it is impossible to violate this
        assert (shares[0].m_hrp == shares[i].m_hrp);
        if (shares[0].m_data[0] != shares[i].m_data[0]) {
            m_valid = MISMATCH_K;
        }
        for (size_t j = 1; j < 5; ++j) {
            if (shares[0].m_data[j] != shares[i].m_data[j]) {
                m_valid = MISMATCH_ID;
            }
        }
        if (shares[i].m_data.size() != shares[0].m_data.size()) {
            m_valid = MISMATCH_LENGTH;
        }

        indices.push_back(shares[i].m_data[5]);
        for (size_t j = i + 1; j < shares.size(); ++j) {
            if (shares[i].m_data[5] == shares[j].m_data[5]) {
                m_valid = DUPLICATE_SHARE;
            }
        }
    }

    m_hrp = shares[0].m_hrp;
    m_data.reserve(shares[0].m_data.size());
    for (size_t j = 0; j < shares[0].m_data.size(); ++j) {
        m_data.push_back(0);
    }

    for (size_t i = 0; i < shares.size(); ++i) {
        uint8_t lagrange_coeff = lagrange_coefficient(indices, shares[i].m_data[5], oidx);
        for (size_t j = 0; j < m_data.size(); ++j) {
            m_data[j] ^= gf32_mul(lagrange_coeff, shares[i].m_data[j]);
        }
    }
}

std::string Result::GetIdString() const {
    assert(IsValid());

    std::string ret;
    ret.reserve(4);
    ret.push_back(bech32::internal::CHARSET[m_data[1]]);
    ret.push_back(bech32::internal::CHARSET[m_data[2]]);
    ret.push_back(bech32::internal::CHARSET[m_data[3]]);
    ret.push_back(bech32::internal::CHARSET[m_data[4]]);
    return ret;
}

size_t Result::GetK() const {
    assert(IsValid());
    return bech32::internal::CHARSET[m_data[0]] - '0';
}

} // namespace codex32
