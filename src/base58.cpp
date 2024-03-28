// Copyright (c) 2014-2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <base58.h>
#include <hash.h>
#include <uint256.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>
#include <cassert>
#include <cstring>

// The Base58 character set excluding "0", "I", "O", and "l" for clarity.
static constexpr auto pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
// Each ASCII character and its Base58 value, with -1 indicating an invalid character for Base58.
static constexpr int8_t mapBase58[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1, 0, 1, 2, 3, 4, 5, 6,  7, 8,-1,-1,-1,-1,-1,-1,
    -1, 9,10,11,12,13,14,15, 16,-1,17,18,19,20,21,-1,
    22,23,24,25,26,27,28,29, 30,31,32,-1,-1,-1,-1,-1,
    -1,33,34,35,36,37,38,39, 40,41,42,43,-1,44,45,46,
    47,48,49,50,51,52,53,54, 55,56,57,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
};

static constexpr int base{58};
static constexpr int baseScale{1000};
static constexpr int log58_256Ratio = 733;  // Approximation of log(base)/log(256), scaled by baseScale.
static constexpr int log256_58Ratio = 1366; // Approximation of log(256)/log(base), scaled by baseScale.

// Defines the size of groups that fit into 64 bit batches, processed together for encoding and decoding efficiency.
static constexpr int encodingBatch = 7;
static constexpr int decodingBatch = 9;
static constexpr int64_t decodingPowerOf58 = static_cast<int64_t>(base)*base*base*base*base*base*base*base*base; // pow(base, decodingBatch)

// The ceiling integer division of x by y.
static int CeilDiv(const int x, const int y)
{
    return (x + (y - 1)) / y;
}

// The floor modulus of x by y, adjusting for negative values.
static int FloorMod(const int x, const int y)
{
    const auto r = x % y;
    return r < 0 ? r + y : r;
}

[[nodiscard]] static bool DecodeBase58(const char* input, std::vector<unsigned char>& result, const int maxRetLen)
{
    while (*input && IsSpace(*input))
        ++input;

    auto leading{0};
    for (; *input == '1'; ++input, ++leading)
        if (leading >= maxRetLen) return false;

    auto effectiveLength{0};
    for (auto p{input}; *p; ++p)
        if (!IsSpace(*p)) ++effectiveLength;

    const auto size = 1 + effectiveLength * log58_256Ratio / baseScale;
    result.reserve(leading + size);
    result.assign(leading, 0x00);

    // Convert the Base58 string to a 64 bit representation for faster manipulation.
    std::vector<int64_t> inputBatched(CeilDiv(effectiveLength, decodingBatch), 0);
    const auto groupOffset = FloorMod(-effectiveLength, decodingBatch);
    for (auto i{0U}; *input && !IsSpace(*input); ++input, ++i) {
        const auto digit = mapBase58[static_cast<uint8_t>(*input)];
        if (digit == -1) return false;
        const auto index = (groupOffset + i) / decodingBatch;
        inputBatched[index] *= base;
        inputBatched[index] += digit;
    }
    for (; *input; ++input)
        if (!IsSpace(*input)) return false; // Ensure no non-space characters after processing.

    auto resultLength{leading};
    for (auto i{0U}; i < inputBatched.size(); ++resultLength) {
        int64_t remainder = 0;
        for (auto j{i}; j < inputBatched.size(); ++j) { // Calculate next digit, dividing inputBatched
            const auto accumulator = (remainder * decodingPowerOf58) + inputBatched[j];
            inputBatched[j] = accumulator / 256;
            remainder = accumulator % 256;
        }
        if (resultLength >= maxRetLen) return false;
        result.push_back(remainder);

        while (i < inputBatched.size() && inputBatched[i] == 0)
            ++i; // Skip new leading zeros
    }

    std::reverse(result.begin() + leading, result.end());

    return true;
}

auto BatchInput(const Span<const unsigned char>& input, const int start) -> std::vector<int64_t>
{
    const int effectiveLength = input.size() - start;
    std::vector<int64_t> inputBatched(CeilDiv(effectiveLength, encodingBatch), 0);
    const int groupOffset = FloorMod(-effectiveLength, encodingBatch) - start;

    for (uint32_t i = start; i < input.size(); ++i) {
        const int index = (groupOffset + static_cast<int>(i)) / encodingBatch;
        inputBatched[index] <<= 8;
        inputBatched[index] |= input[i];
    }

    return inputBatched;
}

std::string EncodeBase58(const Span<const unsigned char> input)
{
    auto leading{0U};
    while (leading < input.size() && input[leading] == 0)
        ++leading;

    const auto size = 1 + input.size() * log256_58Ratio / baseScale;
    std::string result;
    result.reserve(leading + size);
    result.assign(leading, '1'); // Fill in leading '1's for each zero byte in input.

    auto inputBatched = BatchInput(input, leading);
    for (auto i{0U}; i < inputBatched.size();) {
        int64_t remainder{0};
        for (auto j{i}; j < inputBatched.size(); ++j) { // Calculate next digit, dividing inputBatched
            const auto accumulator = (remainder << (encodingBatch * 8)) | inputBatched[j];
            inputBatched[j] = accumulator / base;
            remainder = accumulator % base;
        }
        result += pszBase58[remainder];
        while (i < inputBatched.size() && inputBatched[i] == 0)
            ++i; // Skip new leading zeros
    }
    std::reverse(result.begin() + leading, result.end());
    return result;
}

bool DecodeBase58(const std::string& str, std::vector<unsigned char>& vchRet, int max_ret_len)
{
    if (!ContainsNoNUL(str)) {
        return false;
    }
    return DecodeBase58(str.c_str(), vchRet, max_ret_len);
}

std::string EncodeBase58Check(Span<const unsigned char> input)
{
    // add 4-byte hash check to the end
    std::vector<unsigned char> vch(input.begin(), input.end());
    uint256 hash = Hash(vch);
    vch.insert(vch.end(), (unsigned char*)&hash, (unsigned char*)&hash + 4);
    return EncodeBase58(vch);
}

[[nodiscard]] static bool DecodeBase58Check(const char* psz, std::vector<unsigned char>& vchRet, int max_ret_len)
{
    if (!DecodeBase58(psz, vchRet, max_ret_len > std::numeric_limits<int>::max() - 4 ? std::numeric_limits<int>::max() : max_ret_len + 4) ||
        (vchRet.size() < 4)) {
        vchRet.clear();
        return false;
    }
    // re-calculate the checksum, ensure it matches the included 4-byte checksum
    uint256 hash = Hash(Span{vchRet}.first(vchRet.size() - 4));
    if (memcmp(&hash, &vchRet[vchRet.size() - 4], 4) != 0) {
        vchRet.clear();
        return false;
    }
    vchRet.resize(vchRet.size() - 4);
    return true;
}

bool DecodeBase58Check(const std::string& str, std::vector<unsigned char>& vchRet, int max_ret)
{
    if (!ContainsNoNUL(str)) {
        return false;
    }
    return DecodeBase58Check(str.c_str(), vchRet, max_ret);
}
