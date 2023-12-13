// Copyright (c) 2017, 2021 Pieter Wuille
// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/double_public_key.h>
#include <bech32_mod.h>
#include <util/vector.h>

#include <array>
#include <assert.h>
#include <numeric>
#include <optional>
#include <stdexcept>

namespace bech32_mod
{

namespace
{

typedef std::vector<uint8_t> data;

/** The Bech32 and Bech32m character set for encoding. */
const char* CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

/** The Bech32 and Bech32m character set for decoding. */
const int8_t CHARSET_REV[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    15, -1, 10, 17, 21, 20, 26, 30,  7,  5, -1, -1, -1, -1, -1, -1,
    -1, 29, -1, 24, 13, 25,  9,  8, 23, -1, 18, 22, 31, 27, 19, -1,
     1,  0,  3, 16, 11, 28, 12, 14,  6,  4,  2, -1, -1, -1, -1, -1,
    -1, 29, -1, 24, 13, 25,  9,  8, 23, -1, 18, 22, 31, 27, 19, -1,
     1,  0,  3, 16, 11, 28, 12, 14,  6,  4,  2, -1, -1, -1, -1, -1
};

/* Determine the final constant to use for the specified encoding. */
uint32_t EncodingConstant(Encoding encoding) {
    assert(encoding == Encoding::BECH32 || encoding == Encoding::BECH32M);
    return encoding == Encoding::BECH32 ? 1 : 0x2bc830a3;
}

/** This function will compute what 8 5-bit values to XOR into the last 8 input values, in order to
 *  make the checksum 0. These 8 values are packed together in a single 40-bit integer. The higher
 *  bits correspond to earlier values. */
uint64_t PolyMod(const data& v)
{
    // The input is interpreted as a list of coefficients of a polynomial over F = GF(32), with an
    // implicit 1 in front. If the input is [v0,v1,v2,v3,v4], that polynomial is v(x) =
    // 1*x^7 + v0*x^6 + v1*x^5 + v2*x^4 + v3*x^3 + v4*x^2 + v5*x + v6. The implicit 1 guarantees that
    // [v0,v1,v2,...] has a distinct checksum from [0,v0,v1,v2,...].

    // The output is a 40-bit integer whose 5-bit groups are the coefficients of the remainder of
    // v(x) mod g(x), where g(x) is the Bech32 generator,
    // x^8 + {30}*x^7 + {1}x^6 + {25}*x^5 + {18}*x^4 + {27}*x^3 + {16}*x^2 + {10}*x + {7}. g(x) is
    // chosen in such a way that the resulting code is a BCH code, guaranteeing detection of up to 5
    // errors in a 165-character bech32 string.

    // Note that the coefficients are elements of GF(32), here represented as decimal numbers
    // between {}. In this finite field, addition is just XOR of the corresponding numbers. For
    // example, {27} + {13} = {27 ^ 13} = {22}. Multiplication is more complicated, and requires
    // treating the bits of values themselves as coefficients of a polynomial over a smaller field,
    // GF(2), and multiplying those polynomials mod a^5 + a^3 + 1. For example, {5} * {26} =
    // (a^2 + 1) * (a^4 + a^3 + a) = (a^4 + a^3 + a) * a^2 + (a^4 + a^3 + a) = a^6 + a^5 + a^4 + a
    // = a^3 + 1 (mod a^5 + a^3 + 1) = {9}.

    // During the course of the loop below, `c` contains the bitpacked coefficients of the
    // polynomial constructed from just the values of v that were processed so far, mod g(x). In
    // the above example, `c` initially corresponds to 1 mod g(x), and after processing 2 inputs of
    // v, it corresponds to x^2 + v0*x + v1 mod g(x). As 1 mod g(x) = 1, that is the starting value
    // for `c`.

    // The process of selecting the generator poynomial is discussed in detail in [bech32-mod-gen-poly.md](../doc/bech32-mod-gen-poly.md)

    uint64_t c = 1;
    for (const auto v_i : v) {
        // We want to update `c` to correspond to a polynomial with one extra term. If the initial
        // value of `c` consists of the coefficients of c(x) = f(x) mod g(x), we modify it to
        // correspond to c'(x) = (f(x) * x + v_i) mod g(x), where v_i is the next input to
        // process. Simplifying:
        // c'(x) = (f(x) * x + v_i) mod g(x)
        //         ((f(x) mod g(x)) * x + v_i) mod g(x)
        //         (c(x) * x + v_i) mod g(x)
        // If c(x) = c0*x^7 + c1*x^6 + c2*x^5 + c3*x^4 + c4*x^3 + c5*x^2 + c6*x + c7, we want to compute
        // c'(x) = (c0*x^7 + c1*x^6 + c2*x^5 + c3*x^4 + c4*x^3 + c5*x^2 + c6*x + c7) * x + v_i mod g(x)
        //       = c0*x^8 + c1*x^7 + c2*x^6 + c3*x^5 + c4*x^4 + c5*x^3 + c6*x^2 + c7*x + v_i mod g(x)
        //       = c0*(x^8 mod g(x)) + c1*x^7 + c2*x^6 + c3*x^5 + c4*x^4 + c5*x^3 + c6*x^2 + c7*x + v_i
        // If we call (x^8 mod g(x)) = k(x), this can be written as
        // c'(x) = (c1*x^7 + c2*x^6 + c3*x^5 + c4*x^4 + c5*x^3 + c6*x^2 + c7*x + v_i) + c0*k(x)

        // First, determine the value of c0:
        uint8_t c0 = c >> 35;

        // Then compute c1*x^7 + c2*x^6 + c3*x^5 + c4*x^4 + c5*x^3 + c6*x^2 + c7*x + v_i:
        c = ((c & 0x07ffffffff) << 5) ^ v_i;

        // Finally, for each set bit n in c0, conditionally add {2^n}k(x). These constants can be
        // computed using the following Sage code (continuing the code above):
        //
        // for i in [1,2,4,8,16]: # Print out {1,2,4,8,16}*(g(x) mod x^8), packed in hex integers.
        //     v = 0
        //     for coef in reversed((F.fetch_int(i)*(G % x**8)).coefficients(sparse=True)):
        //         v = v*32 + coef.integer_representation()
        //     print("0x%x" % v)
        //
        if (c0 & 1)  c ^= 0xf0732dc147; // {1}k(x) = {30}*x^7 + {1}x^6 + {25}*x^5 + {18}*x^4 + {27}*x^3 + {16}*x^2 + {10}*x + {7}
        if (c0 & 2)  c ^= 0xa8b6dfa68e; // {2}k(x)
        if (c0 & 4)  c ^= 0x193fabc83c; // {4}k(x)
        if (c0 & 8)  c ^= 0x322fd3b451; // {8}k(x)
        if (c0 & 16)  c ^= 0x640f37688b; // {16}k(x)
    }
    return c;
}

/** Convert to lower case. */
inline unsigned char LowerCase(unsigned char c)
{
    return (c >= 'A' && c <= 'Z') ? (c - 'A') + 'a' : c;
}

/** Return indices of invalid characters in a Bech32 string. */
bool CheckCharacters(const std::string& str, std::vector<int>& errors)
{
    bool lower = false, upper = false;
    for (size_t i = 0; i < str.size(); ++i) {
        unsigned char c{(unsigned char)(str[i])};
        if (c >= 'a' && c <= 'z') {
            if (upper) {
                errors.push_back(i);
            } else {
                lower = true;
            }
        } else if (c >= 'A' && c <= 'Z') {
            if (lower) {
                errors.push_back(i);
            } else {
                upper = true;
            }
        } else if (c < 33 || c > 126) {
            errors.push_back(i);
        }
    }
    return errors.empty();
}

/** Expand a HRP for use in checksum computation. */
data ExpandHRP(const std::string& hrp)
{
    data ret;
    ret.reserve(hrp.size() + 154 + 5 + 1);  // 154=data part, 5=expanded 2-char hrp, 1=separator
    ret.resize(hrp.size() * 2 + 1);
    for (size_t i = 0; i < hrp.size(); ++i) {
        unsigned char c = hrp[i];
        ret[i] = c >> 5;
        ret[i + hrp.size() + 1] = c & 0x1f;
    }
    ret[hrp.size()] = 0;
    return ret;
}

/** Verify a checksum. */
Encoding VerifyChecksum(const std::string& hrp, const data& values)
{
    // PolyMod computes what value to xor into the final values to make the checksum 0. However,
    // if we required that the checksum was 0, it would be the case that appending a 0 to a valid
    // list of values would result in a new valid list. For that reason, Bech32 requires the
    // resulting checksum to be 1 instead. In Bech32m, this constant was amended. See
    // https://gist.github.com/sipa/14c248c288c3880a3b191f978a34508e for details.
    const uint64_t check = PolyMod(Cat(ExpandHRP(hrp), values));
    if (check == EncodingConstant(Encoding::BECH32)) return Encoding::BECH32;
    if (check == EncodingConstant(Encoding::BECH32M)) return Encoding::BECH32M;
    return Encoding::INVALID;
}

/** Create a checksum. */
data CreateChecksum(Encoding encoding, const std::string& hrp, const data& values)
{
    auto exp_hrp = ExpandHRP(hrp);
    data enc = Cat(ExpandHRP(hrp), values);
    enc.resize(enc.size() + 8); // Append 8 zeroes
    uint64_t mod = PolyMod(enc) ^ EncodingConstant(encoding); // Determine what to XOR into those 8 zeroes.
    data ret(8);
    for (size_t i = 0; i < 8; ++i) {
        // Convert the 5-bit groups in mod to checksum values.
        ret[i] = (mod >> (5 * (7 - i))) & 31;
    }
    return ret;
}

} // namespace

/** Encode a Bech32 or Bech32m string. */
std::string Encode(Encoding encoding, const std::string& hrp, const data& values) {
    if (values.size() != DOUBLE_PUBKEY_DATA_ENC_SIZE) {
        throw std::runtime_error("Expected values to be a double public key");
    }
    // First ensure that the HRP is all lowercase. BIP-173 and BIP350 require an encoder
    // to return a lowercase Bech32/Bech32m string, but if given an uppercase HRP, the
    // result will always be invalid.
    for (const char& c : hrp) assert(c < 'A' || c > 'Z');
    data checksum = CreateChecksum(encoding, hrp, values);
    data combined = Cat(values, checksum);
    std::string ret = hrp + '1';
    ret.reserve(ret.size() + combined.size());
    for (const auto c : combined) {
        ret += CHARSET[c];
    }
    return ret;
}

/** Decode a Bech32 or Bech32m string. Expects
 *  str to be a valid encoding of DoublePublicKey */
DecodeResult Decode(const std::string& str) {
    std::vector<int> errors;
    if (!CheckCharacters(str, errors)) return {};
    size_t pos = str.rfind('1');

    data values(str.size() - 1 - pos);
    for (size_t i = 0; i < str.size() - 1 - pos; ++i) {
        unsigned char c = str[i + pos + 1];
        int8_t rev = CHARSET_REV[c];

        if (rev == -1) {
            return {};
        }
        values[i] = rev;
    }
    std::string hrp;
    for (size_t i = 0; i < pos; ++i) {
        hrp += LowerCase(str[i]);
    }
    Encoding result = VerifyChecksum(hrp, values);
    if (result == Encoding::INVALID) return {};
    return {result, std::move(hrp), data(values.begin(), values.end() - 8)};
}

} // namespace bech32_mod

