// Copyright (c) 2017, 2021 Pieter Wuille
// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bech32.h>
#include <util/vector.h>

#include <array>
#include <assert.h>
#include <numeric>
#include <optional>

namespace bech32
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

/** We work with the finite field GF(1024) defined as a degree 2 extension of the base field GF(32)
 * The defining polynomial of the extension is x^2 + 9x + 23.
 * Let (e) be a root of this defining polynomial. Then (e) is a primitive element of GF(1024),
 * that is, a generator of the field. Every non-zero element of the field can then be represented
 * as (e)^k for some power k.
 * The array GF1024_EXP contains all these powers of (e) - GF1024_EXP[k] = (e)^k in GF(1024).
 * Conversely, GF1024_LOG contains the discrete logarithms of these powers, so
 * GF1024_LOG[GF1024_EXP[k]] == k.
 * The following function generates the two tables GF1024_EXP and GF1024_LOG as constexprs. */
constexpr std::pair<std::array<int16_t, 1023>, std::array<int16_t, 1024>> GenerateGFTables()
{
    // Build table for GF(32).
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

    // Build table for GF(1024)
    std::array<int16_t, 1023> GF1024_EXP{};
    std::array<int16_t, 1024> GF1024_LOG{};

    GF1024_EXP[0] = 1;
    GF1024_LOG[0] = -1;
    GF1024_LOG[1] = 0;

    // Each element v of GF(1024) is encoded as a 10 bit integer in the following way:
    // v = v1 || v0 where v0, v1 are 5-bit integers (elements of GF(32)).
    // The element (e) is encoded as 1 || 0, to represent 1*(e) + 0. Every other element
    // a*(e) + b is represented as a || b (a and b are both GF(32) elements). Given (v),
    // we compute (e)*(v) by multiplying in the following way:
    //
    // v0' = 23*v1
    // v1' = 9*v1 + v0
    // e*v = v1' || v0'
    //
    // Where 23, 9 are GF(32) elements encoded as described above. Multiplication in GF(32)
    // is done using the log/exp tables:
    // e^x * e^y = e^(x + y) so a * b = EXP[ LOG[a] + LOG [b] ]
    // for non-zero a and b.

    v = 1;
    for (int i = 1; i < 1023; ++i) {
        int v0 = v & 31;
        int v1 = v >> 5;

        int v0n = v1 ? GF32_EXP.at((GF32_LOG.at(v1) + GF32_LOG.at(23)) % 31) : 0;
        int v1n = (v1 ? GF32_EXP.at((GF32_LOG.at(v1) + GF32_LOG.at(9)) % 31) : 0) ^ v0;

        v = v1n << 5 | v0n;
        GF1024_EXP[i] = v;
        GF1024_LOG[v] = i;
    }

    return std::make_pair(GF1024_EXP, GF1024_LOG);
}

constexpr auto tables = GenerateGFTables();
constexpr const std::array<int16_t, 1023>& GF1024_EXP = tables.first;
constexpr const std::array<int16_t, 1024>& GF1024_LOG = tables.second;

/* Determine the final constant to use for the specified encoding. */
uint32_t EncodingConstant(Encoding encoding) {
    assert(encoding == Encoding::BECH32 || encoding == Encoding::BECH32M);
    return encoding == Encoding::BECH32 ? 1 : 0x2bc830a3;
}

/** This function will compute what 6 5-bit values to XOR into the last 6 input values, in order to
 *  make the checksum 0. These 6 values are packed together in a single 30-bit integer. The higher
 *  bits correspond to earlier values. */
uint32_t PolyMod(const data& v)
{
    // The input is interpreted as a list of coefficients of a polynomial over F = GF(32), with an
    // implicit 1 in front. If the input is [v0,v1,v2,v3,v4], that polynomial is v(x) =
    // 1*x^5 + v0*x^4 + v1*x^3 + v2*x^2 + v3*x + v4. The implicit 1 guarantees that
    // [v0,v1,v2,...] has a distinct checksum from [0,v0,v1,v2,...].

    // The output is a 30-bit integer whose 5-bit groups are the coefficients of the remainder of
    // v(x) mod g(x), where g(x) is the Bech32 generator,
    // x^6 + {29}x^5 + {22}x^4 + {20}x^3 + {21}x^2 + {29}x + {18}. g(x) is chosen in such a way
    // that the resulting code is a BCH code, guaranteeing detection of up to 3 errors within a
    // window of 1023 characters. Among the various possible BCH codes, one was selected to in
    // fact guarantee detection of up to 4 errors within a window of 89 characters.

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

    // The following Sage code constructs the generator used:
    //
    // B = GF(2) # Binary field
    // BP.<b> = B[] # Polynomials over the binary field
    // F_mod = b**5 + b**3 + 1
    // F.<f> = GF(32, modulus=F_mod, repr='int') # GF(32) definition
    // FP.<x> = F[] # Polynomials over GF(32)
    // E_mod = x**2 + F.fetch_int(9)*x + F.fetch_int(23)
    // E.<e> = F.extension(E_mod) # GF(1024) extension field definition
    // for p in divisors(E.order() - 1): # Verify e has order 1023.
    //    assert((e**p == 1) == (p % 1023 == 0))
    // G = lcm([(e**i).minpoly() for i in range(997,1000)])
    // print(G) # Print out the generator
    //
    // It demonstrates that g(x) is the least common multiple of the minimal polynomials
    // of 3 consecutive powers (997,998,999) of a primitive element (e) of GF(1024).
    // That guarantees it is, in fact, the generator of a primitive BCH code with cycle
    // length 1023 and distance 4. See https://en.wikipedia.org/wiki/BCH_code for more details.

    uint32_t c = 1;
    for (const auto v_i : v) {
        // We want to update `c` to correspond to a polynomial with one extra term. If the initial
        // value of `c` consists of the coefficients of c(x) = f(x) mod g(x), we modify it to
        // correspond to c'(x) = (f(x) * x + v_i) mod g(x), where v_i is the next input to
        // process. Simplifying:
        // c'(x) = (f(x) * x + v_i) mod g(x)
        //         ((f(x) mod g(x)) * x + v_i) mod g(x)
        //         (c(x) * x + v_i) mod g(x)
        // If c(x) = c0*x^5 + c1*x^4 + c2*x^3 + c3*x^2 + c4*x + c5, we want to compute
        // c'(x) = (c0*x^5 + c1*x^4 + c2*x^3 + c3*x^2 + c4*x + c5) * x + v_i mod g(x)
        //       = c0*x^6 + c1*x^5 + c2*x^4 + c3*x^3 + c4*x^2 + c5*x + v_i mod g(x)
        //       = c0*(x^6 mod g(x)) + c1*x^5 + c2*x^4 + c3*x^3 + c4*x^2 + c5*x + v_i
        // If we call (x^6 mod g(x)) = k(x), this can be written as
        // c'(x) = (c1*x^5 + c2*x^4 + c3*x^3 + c4*x^2 + c5*x + v_i) + c0*k(x)

        // First, determine the value of c0:
        uint8_t c0 = c >> 25;

        // Then compute c1*x^5 + c2*x^4 + c3*x^3 + c4*x^2 + c5*x + v_i:
        c = ((c & 0x1ffffff) << 5) ^ v_i;

        // Finally, for each set bit n in c0, conditionally add {2^n}k(x). These constants can be
        // computed using the following Sage code (continuing the code above):
        //
        // for i in [1,2,4,8,16]: # Print out {1,2,4,8,16}*(g(x) mod x^6), packed in hex integers.
        //     v = 0
        //     for coef in reversed((F.fetch_int(i)*(G % x**6)).coefficients(sparse=True)):
        //         v = v*32 + coef.integer_representation()
        //     print("0x%x" % v)
        //
        if (c0 & 1)  c ^= 0x3b6a57b2; //     k(x) = {29}x^5 + {22}x^4 + {20}x^3 + {21}x^2 + {29}x + {18}
        if (c0 & 2)  c ^= 0x26508e6d; //  {2}k(x) = {19}x^5 +  {5}x^4 +     x^3 +  {3}x^2 + {19}x + {13}
        if (c0 & 4)  c ^= 0x1ea119fa; //  {4}k(x) = {15}x^5 + {10}x^4 +  {2}x^3 +  {6}x^2 + {15}x + {26}
        if (c0 & 8)  c ^= 0x3d4233dd; //  {8}k(x) = {30}x^5 + {20}x^4 +  {4}x^3 + {12}x^2 + {30}x + {29}
        if (c0 & 16) c ^= 0x2a1462b3; // {16}k(x) = {21}x^5 +     x^4 +  {8}x^3 + {24}x^2 + {21}x + {19}

    }
    return c;
}

/** Syndrome computes the values s_j = R(e^j) for j in [997, 998, 999]. As described above, the
 * generator polynomial G is the LCM of the minimal polynomials of (e)^997, (e)^998, and (e)^999.
 *
 * Consider a codeword with errors, of the form R(x) = C(x) + E(x). The residue is the bit-packed
 * result of computing R(x) mod G(X), where G is the generator of the code. Because C(x) is a valid
 * codeword, it is a multiple of G(X), so the residue is in fact just E(x) mod G(x). Note that all
 * of the (e)^j are roots of G(x) by definition, so R((e)^j) = E((e)^j).
 *
 * Let R(x) = r1*x^5 + r2*x^4 + r3*x^3 + r4*x^2 + r5*x + r6
 *
 * To compute R((e)^j), we are really computing:
 * r1*(e)^(j*5) + r2*(e)^(j*4) + r3*(e)^(j*3) + r4*(e)^(j*2) + r5*(e)^j + r6
 *
 * Now note that all of the (e)^(j*i) for i in [5..0] are constants and can be precomputed.
 * But even more than that, we can consider each coefficient as a bit-string.
 * For example, take r5 = (b_5, b_4, b_3, b_2, b_1) written out as 5 bits. Then:
 * r5*(e)^j = b_1*(e)^j + b_2*(2*(e)^j) + b_3*(4*(e)^j) + b_4*(8*(e)^j) + b_5*(16*(e)^j)
 * where all the (2^i*(e)^j) are constants and can be precomputed.
 *
 * Then we just add each of these corresponding constants to our final value based on the
 * bit values b_i. This is exactly what is done in the Syndrome function below.
 */
constexpr std::array<uint32_t, 25> GenerateSyndromeConstants() {
    std::array<uint32_t, 25> SYNDROME_CONSTS{};
    for (int k = 1; k < 6; ++k) {
        for (int shift = 0; shift < 5; ++shift) {
            int16_t b = GF1024_LOG.at(size_t{1} << shift);
            int16_t c0 = GF1024_EXP.at((997*k + b) % 1023);
            int16_t c1 = GF1024_EXP.at((998*k + b) % 1023);
            int16_t c2 = GF1024_EXP.at((999*k + b) % 1023);
            uint32_t c = c2 << 20 | c1 << 10 | c0;
            int ind = 5*(k-1) + shift;
            SYNDROME_CONSTS[ind] = c;
        }
    }
    return SYNDROME_CONSTS;
}
constexpr std::array<uint32_t, 25> SYNDROME_CONSTS = GenerateSyndromeConstants();

/**
 * Syndrome returns the three values s_997, s_998, and s_999 described above,
 * packed into a 30-bit integer, where each group of 10 bits encodes one value.
 */
uint32_t Syndrome(const uint32_t residue) {
    // low is the first 5 bits, corresponding to the r6 in the residue
    // (the constant term of the polynomial).
    uint32_t low = residue & 0x1f;

    // We begin by setting s_j = low = r6 for all three values of j, because these are unconditional.
    uint32_t result = low ^ (low << 10) ^ (low << 20);

    // Then for each following bit, we add the corresponding precomputed constant if the bit is 1.
    // For example, 0x31edd3c4 is 1100011110 1101110100 1111000100 when unpacked in groups of 10
    // bits, corresponding exactly to a^999 || a^998 || a^997 (matching the corresponding values in
    // GF1024_EXP above). In this way, we compute all three values of s_j for j in (997, 998, 999)
    // simultaneously. Recall that XOR corresponds to addition in a characteristic 2 field.
    for (int i = 0; i < 25; ++i) {
        result ^= ((residue >> (5+i)) & 1 ? SYNDROME_CONSTS.at(i) : 0);
    }
    return result;
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

std::vector<unsigned char> PreparePolynomialCoefficients(const std::string& hrp, const data& values)
{
    data ret;
    ret.reserve(hrp.size() + 1 + hrp.size() + values.size() + CHECKSUM_SIZE);

    /** Expand a HRP for use in checksum computation. */
    for (size_t i = 0; i < hrp.size(); ++i) ret.push_back(hrp[i] >> 5);
    ret.push_back(0);
    for (size_t i = 0; i < hrp.size(); ++i) ret.push_back(hrp[i] & 0x1f);

    ret.insert(ret.end(), values.begin(), values.end());

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
    auto enc = PreparePolynomialCoefficients(hrp, values);
    const uint32_t check = PolyMod(enc);
    if (check == EncodingConstant(Encoding::BECH32)) return Encoding::BECH32;
    if (check == EncodingConstant(Encoding::BECH32M)) return Encoding::BECH32M;
    return Encoding::INVALID;
}

/** Create a checksum. */
data CreateChecksum(Encoding encoding, const std::string& hrp, const data& values)
{
    auto enc = PreparePolynomialCoefficients(hrp, values);
    enc.insert(enc.end(), CHECKSUM_SIZE, 0x00);
    uint32_t mod = PolyMod(enc) ^ EncodingConstant(encoding); // Determine what to XOR into those 6 zeroes.
    data ret(CHECKSUM_SIZE);
    for (size_t i = 0; i < CHECKSUM_SIZE; ++i) {
        // Convert the 5-bit groups in mod to checksum values.
        ret[i] = (mod >> (5 * (5 - i))) & 31;
    }
    return ret;
}

} // namespace

/** Encode a Bech32 or Bech32m string. */
std::string Encode(Encoding encoding, const std::string& hrp, const data& values) {
    // First ensure that the HRP is all lowercase. BIP-173 and BIP350 require an encoder
    // to return a lowercase Bech32/Bech32m string, but if given an uppercase HRP, the
    // result will always be invalid.
    for (const char& c : hrp) assert(c < 'A' || c > 'Z');

    std::string ret;
    ret.reserve(hrp.size() + 1 + values.size() + CHECKSUM_SIZE);
    ret += hrp;
    ret += '1';
    for (const uint8_t& i : values) ret += CHARSET[i];
    for (const uint8_t& i : CreateChecksum(encoding, hrp, values)) ret += CHARSET[i];
    return ret;
}

/** Decode a Bech32 or Bech32m string. */
DecodeResult Decode(const std::string& str, CharLimit limit) {
    std::vector<int> errors;
    if (!CheckCharacters(str, errors)) return {};
    size_t pos = str.rfind('1');
    if (str.size() > limit) return {};
    if (pos == str.npos || pos == 0 || pos + CHECKSUM_SIZE >= str.size()) {
        return {};
    }
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
    hrp.reserve(pos);
    for (size_t i = 0; i < pos; ++i) {
        hrp += LowerCase(str[i]);
    }
    Encoding result = VerifyChecksum(hrp, values);
    if (result == Encoding::INVALID) return {};
    return {result, std::move(hrp), data(values.begin(), values.end() - CHECKSUM_SIZE)};
}

/** Find index of an incorrect character in a Bech32 string. */
std::pair<std::string, std::vector<int>> LocateErrors(const std::string& str, CharLimit limit) {
    std::vector<int> error_locations{};

    if (str.size() > limit) {
        error_locations.resize(str.size() - limit);
        std::iota(error_locations.begin(), error_locations.end(), static_cast<int>(limit));
        return std::make_pair("Bech32 string too long", std::move(error_locations));
    }

    if (!CheckCharacters(str, error_locations)){
        return std::make_pair("Invalid character or mixed case", std::move(error_locations));
    }

    size_t pos = str.rfind('1');
    if (pos == str.npos) {
        return std::make_pair("Missing separator", std::vector<int>{});
    }
    if (pos == 0 || pos + CHECKSUM_SIZE >= str.size()) {
        error_locations.push_back(pos);
        return std::make_pair("Invalid separator position", std::move(error_locations));
    }

    std::string hrp;
    hrp.reserve(pos);
    for (size_t i = 0; i < pos; ++i) {
        hrp += LowerCase(str[i]);
    }

    size_t length = str.size() - 1 - pos; // length of data part
    data values(length);
    for (size_t i = pos + 1; i < str.size(); ++i) {
        unsigned char c = str[i];
        int8_t rev = CHARSET_REV[c];
        if (rev == -1) {
            error_locations.push_back(i);
            return std::make_pair("Invalid Base 32 character", std::move(error_locations));
        }
        values[i - pos - 1] = rev;
    }

    // We attempt error detection with both bech32 and bech32m, and choose the one with the fewest errors
    // We can't simply use the segwit version, because that may be one of the errors
    std::optional<Encoding> error_encoding;
    for (Encoding encoding : {Encoding::BECH32, Encoding::BECH32M}) {
        std::vector<int> possible_errors;
        // Recall that (expanded hrp + values) is interpreted as a list of coefficients of a polynomial
        // over GF(32). PolyMod computes the "remainder" of this polynomial modulo the generator G(x).
        auto enc = PreparePolynomialCoefficients(hrp, values);
        uint32_t residue = PolyMod(enc) ^ EncodingConstant(encoding);

        // All valid codewords should be multiples of G(x), so this remainder (after XORing with the encoding
        // constant) should be 0 - hence 0 indicates there are no errors present.
        if (residue != 0) {
            // If errors are present, our polynomial must be of the form C(x) + E(x) where C is the valid
            // codeword (a multiple of G(x)), and E encodes the errors.
            uint32_t syn = Syndrome(residue);

            // Unpack the three 10-bit syndrome values
            int s0 = syn & 0x3FF;
            int s1 = (syn >> 10) & 0x3FF;
            int s2 = syn >> 20;

            // Get the discrete logs of these values in GF1024 for more efficient computation
            int l_s0 = GF1024_LOG.at(s0);
            int l_s1 = GF1024_LOG.at(s1);
            int l_s2 = GF1024_LOG.at(s2);

            // First, suppose there is only a single error. Then E(x) = e1*x^p1 for some position p1
            // Then s0 = E((e)^997) = e1*(e)^(997*p1) and s1 = E((e)^998) = e1*(e)^(998*p1)
            // Therefore s1/s0 = (e)^p1, and by the same logic, s2/s1 = (e)^p1 too.
            // Hence, s1^2 == s0*s2, which is exactly the condition we check first:
            if (l_s0 != -1 && l_s1 != -1 && l_s2 != -1 && (2 * l_s1 - l_s2 - l_s0 + 2046) % 1023 == 0) {
                // Compute the error position p1 as l_s1 - l_s0 = p1 (mod 1023)
                size_t p1 = (l_s1 - l_s0 + 1023) % 1023; // the +1023 ensures it is positive
                // Now because s0 = e1*(e)^(997*p1), we get e1 = s0/((e)^(997*p1)). Remember that (e)^1023 = 1,
                // so 1/((e)^997) = (e)^(1023-997).
                int l_e1 = l_s0 + (1023 - 997) * p1;
                // Finally, some sanity checks on the result:
                // - The error position should be within the length of the data
                // - e1 should be in GF(32), which implies that e1 = (e)^(33k) for some k (the 31 non-zero elements
                // of GF(32) form an index 33 subgroup of the 1023 non-zero elements of GF(1024)).
                if (p1 < length && !(l_e1 % 33)) {
                    // Polynomials run from highest power to lowest, so the index p1 is from the right.
                    // We don't return e1 because it is dangerous to suggest corrections to the user,
                    // the user should check the address themselves.
                    possible_errors.push_back(str.size() - p1 - 1);
                }
            // Otherwise, suppose there are two errors. Then E(x) = e1*x^p1 + e2*x^p2.
            } else {
                // For all possible first error positions p1
                for (size_t p1 = 0; p1 < length; ++p1) {
                    // We have guessed p1, and want to solve for p2. Recall that E(x) = e1*x^p1 + e2*x^p2, so
                    // s0 = E((e)^997) = e1*(e)^(997^p1) + e2*(e)^(997*p2), and similar for s1 and s2.
                    //
                    // Consider s2 + s1*(e)^p1
                    //          = 2e1*(e)^(999^p1) + e2*(e)^(999*p2) + e2*(e)^(998*p2)*(e)^p1
                    //          = e2*(e)^(999*p2) + e2*(e)^(998*p2)*(e)^p1
                    //    (Because we are working in characteristic 2.)
                    //          = e2*(e)^(998*p2) ((e)^p2 + (e)^p1)
                    //
                    int s2_s1p1 = s2 ^ (s1 == 0 ? 0 : GF1024_EXP.at((l_s1 + p1) % 1023));
                    if (s2_s1p1 == 0) continue;
                    int l_s2_s1p1 = GF1024_LOG.at(s2_s1p1);

                    // Similarly, s1 + s0*(e)^p1
                    //          = e2*(e)^(997*p2) ((e)^p2 + (e)^p1)
                    int s1_s0p1 = s1 ^ (s0 == 0 ? 0 : GF1024_EXP.at((l_s0 + p1) % 1023));
                    if (s1_s0p1 == 0) continue;
                    int l_s1_s0p1 = GF1024_LOG.at(s1_s0p1);

                    // So, putting these together, we can compute the second error position as
                    // (e)^p2 = (s2 + s1^p1)/(s1 + s0^p1)
                    // p2 = log((e)^p2)
                    size_t p2 = (l_s2_s1p1 - l_s1_s0p1 + 1023) % 1023;

                    // Sanity checks that p2 is a valid position and not the same as p1
                    if (p2 >= length || p1 == p2) continue;

                    // Now we want to compute the error values e1 and e2.
                    // Similar to above, we compute s1 + s0*(e)^p2
                    //          = e1*(e)^(997*p1) ((e)^p1 + (e)^p2)
                    int s1_s0p2 = s1 ^ (s0 == 0 ? 0 : GF1024_EXP.at((l_s0 + p2) % 1023));
                    if (s1_s0p2 == 0) continue;
                    int l_s1_s0p2 = GF1024_LOG.at(s1_s0p2);

                    // And compute (the log of) 1/((e)^p1 + (e)^p2))
                    int inv_p1_p2 = 1023 - GF1024_LOG.at(GF1024_EXP.at(p1) ^ GF1024_EXP.at(p2));

                    // Then (s1 + s0*(e)^p1) * (1/((e)^p1 + (e)^p2)))
                    //         = e2*(e)^(997*p2)
                    // Then recover e2 by dividing by (e)^(997*p2)
                    int l_e2 = l_s1_s0p1 + inv_p1_p2 + (1023 - 997) * p2;
                    // Check that e2 is in GF(32)
                    if (l_e2 % 33) continue;

                    // In the same way, (s1 + s0*(e)^p2) * (1/((e)^p1 + (e)^p2)))
                    //         = e1*(e)^(997*p1)
                    // So recover e1 by dividing by (e)^(997*p1)
                    int l_e1 = l_s1_s0p2 + inv_p1_p2 + (1023 - 997) * p1;
                    // Check that e1 is in GF(32)
                    if (l_e1 % 33) continue;

                    // Again, we do not return e1 or e2 for safety.
                    // Order the error positions from the left of the string and return them
                    if (p1 > p2) {
                        possible_errors.push_back(str.size() - p1 - 1);
                        possible_errors.push_back(str.size() - p2 - 1);
                    } else {
                        possible_errors.push_back(str.size() - p2 - 1);
                        possible_errors.push_back(str.size() - p1 - 1);
                    }
                    break;
                }
            }
        } else {
            // No errors
            return std::make_pair("", std::vector<int>{});
        }

        if (error_locations.empty() || (!possible_errors.empty() && possible_errors.size() < error_locations.size())) {
            error_locations = std::move(possible_errors);
            if (!error_locations.empty()) error_encoding = encoding;
        }
    }
    std::string error_message = error_encoding == Encoding::BECH32M ? "Invalid Bech32m checksum"
                              : error_encoding == Encoding::BECH32 ? "Invalid Bech32 checksum"
                              : "Invalid checksum";

    return std::make_pair(error_message, std::move(error_locations));
}

} // namespace bech32
