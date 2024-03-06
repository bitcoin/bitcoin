// Copyright (c) 2014-2022 The Bitcoin Core developers
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

using util::ContainsNoNUL;

/** All alphanumeric characters except for "0", "I", "O", and "l" */
static constexpr const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
/** Each ASCII character and its Base58 value, with -1 indicating an invalid character for Base58 */
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
static constexpr int64_t baseScale{1000LL};
static constexpr int64_t log58_256Ratio =  733LL; // Approximation of log(base)/log(256), scaled by baseScale.
static constexpr int64_t log256_58Ratio = 1366LL; // Approximation of log(256)/log(base), scaled by baseScale.

// Defines the size of groups that fit into 64 bit batches, processed together for encoding and decoding efficiency.
static constexpr int encodingBatch = 7;
static constexpr int decodingBatch = 9;
static constexpr int64_t decodingPowerOf58 = static_cast<int64_t>(base)*base*base*base*base*base*base*base*base; // pow(base, decodingBatch)

[[nodiscard]] static bool DecodeBase58(const char* input, std::vector<unsigned char>& result, const int max_ret_len)
{
    while (*input && IsSpace(*input))
        ++input;

    size_t leading{0};
    for (; *input == '1'; ++input, ++leading)
        if (leading >= static_cast<size_t>(max_ret_len)) [[unlikely]] return false;

    size_t effectiveLength{0};
    for (const char* p{input}; *p; ++p)
        if (!IsSpace(*p)) ++effectiveLength;

    const size_t size = 1 + effectiveLength * log58_256Ratio / baseScale;
    result.reserve(leading + size);
    result.assign(leading, 0x00);

    // Convert the Base58 string to a 64 bit representation for faster manipulation.
    std::vector<int64_t> inputBatched((effectiveLength + decodingBatch - 1) / decodingBatch, 0);
    const size_t groupOffset = (decodingBatch - (effectiveLength % decodingBatch)) % decodingBatch;
    for (size_t i{0}; *input && !IsSpace(*input); ++input, ++i) {
        const int8_t digit = mapBase58[static_cast<uint8_t>(*input)];
        if (digit == -1) [[unlikely]] return false;
        const size_t index = (groupOffset + i) / decodingBatch;
        inputBatched[index] = (inputBatched[index] * base) + digit;
    }
    for (; *input; ++input)
        if (!IsSpace(*input)) [[unlikely]] return false; // Ensure no non-space characters after processing.

    size_t resultLength{leading};
    for (size_t i{0}; i < inputBatched.size(); ++resultLength) {
        int64_t remainder = 0;
        for (size_t j{i}; j < inputBatched.size(); ++j) { // Calculate next digit, dividing inputBatched
            const int64_t accumulator = (remainder * decodingPowerOf58) + inputBatched[j];
            inputBatched[j] = accumulator / 256;
            remainder = accumulator % 256;
        }
        if (resultLength >= static_cast<size_t>(max_ret_len)) [[unlikely]] return false;
        result.push_back(remainder);

        while (i < inputBatched.size() && inputBatched[i] == 0)
            ++i; // Skip new leading zeros
    }
    std::reverse(result.begin() + leading, result.end());
    return true;
}

std::string EncodeBase58(const Span<const unsigned char> input)
{
    size_t leading{0};
    while (leading < input.size() && input[leading] == 0)
        ++leading;

    std::string result;
    const size_t size = 1 + input.size() * log256_58Ratio / baseScale;
    result.reserve(leading + size);
    result.assign(leading, '1'); // Fill in leading '1's for each zero byte in input.

    const size_t effectiveLength = input.size() - leading;
    std::vector<int64_t> inputBatched((effectiveLength + encodingBatch - 1) / encodingBatch, 0);
    const size_t groupOffset = (encodingBatch - (effectiveLength % encodingBatch)) % encodingBatch;
    for (size_t i = leading; i < input.size(); ++i) {
        const size_t index = (i - leading + groupOffset) / encodingBatch;
        inputBatched[index] = (inputBatched[index] << 8) | input[i];
    }
    for (size_t i{0}; i < inputBatched.size();) {
        int64_t remainder{0};
        for (size_t j{i}; j < inputBatched.size(); ++j) { // Calculate next digit, dividing inputBatched
            const int64_t accumulator = (remainder << (encodingBatch * 8)) | inputBatched[j];
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
