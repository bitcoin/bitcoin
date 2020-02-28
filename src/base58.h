// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
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
#ifndef BITCOIN_BASE58_H
#define BITCOIN_BASE58_H

#include <attributes.h>

#include <string>
#include <vector>

/**
 * Encode a byte vector as a base58-encoded string
 */
NODISCARD std::string EncodeBase58(const std::vector<unsigned char>& vch);

/**
 * Encode a byte vector into a base58-encoded string, including checksum
 */
NODISCARD std::string EncodeBase58Check(const std::vector<unsigned char>& vchIn);

/**
 * Decode a base58-encoded string (psz) into a byte vector (vchRet).
 * psz cannot be nullptr.
 * @returns true if decoding is successful.
 */
NODISCARD bool DecodeBase58(const char* psz, std::vector<unsigned char>& vchRet, int max_ret_len);

/**
 * Decode a base58-encoded string (str) that includes a checksum into a byte
 * vector (vchRet), return true if decoding is successful
 */
NODISCARD bool DecodeBase58Check(const std::string& str, std::vector<unsigned char>& vchRet, int max_ret_len);

#endif // BITCOIN_BASE58_H
