// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Why base-58 instead of standard base-64 encoding?
 * - Don't want 0OIl characters that look the same in some fonts and
 *      could be used to create visually identical looking data.
 * - A string with non-alphanumeric characters is not as easily accepted as input.
 * - E-mail usually won't line-break if there's no punctuation to break at.
 * - Double-clicking selects the whole string as one word if it's all alphanumeric.
 */
#ifndef SYSCOIN_BASE58_H
#define SYSCOIN_BASE58_H

#include <span.h>

#include <string>
#include <vector>

/**
 * Encode a byte span as a base58-encoded string
 */
std::string EncodeBase58(Span<const unsigned char> input);

/**
 * Decode a base58-encoded string (str) into a byte vector (vchRet).
 * return true if decoding is successful.
 */
[[nodiscard]] bool DecodeBase58(const std::string& str, std::vector<unsigned char>& vchRet, int max_ret_len);

/**
 * Encode a byte span into a base58-encoded string, including checksum
 */
std::string EncodeBase58Check(Span<const unsigned char> input);

/**
 * Decode a base58-encoded string (str) that includes a checksum into a byte
 * vector (vchRet), return true if decoding is successful
 */
[[nodiscard]] bool DecodeBase58Check(const std::string& str, std::vector<unsigned char>& vchRet, int max_ret_len);

#endif // SYSCOIN_BASE58_H
