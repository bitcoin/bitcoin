// Copyright (c) 2017, 2021 Pieter Wuille
// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Bech32 and Bech32m are string encoding formats used in newer
// address types. The outputs consist of a human-readable part
// (alphanumeric), a separator character (1), and a base32 data
// section, the last 6 characters of which are a checksum. The
// module is namespaced under bech32 for historical reasons.
//
// For more information, see BIP 173 and BIP 350.

#ifndef BITCOIN_BECH32_H
#define BITCOIN_BECH32_H

#include <stdint.h>
#include <string>
#include <vector>

namespace bech32
{

static constexpr size_t CHECKSUM_SIZE = 6;
static constexpr char SEPARATOR = '1';

enum class Encoding {
    INVALID, //!< Failed decoding

    BECH32,  //!< Bech32 encoding as defined in BIP173
    BECH32M, //!< Bech32m encoding as defined in BIP350
};

/** Character limits for Bech32(m) encoded strings. Character limits are how we provide error location guarantees.
 *  These values should never exceed 2^31 - 1 (max value for a 32-bit int), since there are places where we may need to
 *  convert the CharLimit::VALUE to an int. In practice, this should never happen since this CharLimit applies to an address encoding
 *  and we would never encode an address with such a massive value */
enum CharLimit : size_t {
    BECH32 = 90,            //!< BIP173/350 imposed character limit for Bech32(m) encoded addresses. This guarantees finding up to 4 errors.
};

/** Encode a Bech32 or Bech32m string. If hrp contains uppercase characters, this will cause an
 *  assertion error. Encoding must be one of BECH32 or BECH32M. */
std::string Encode(Encoding encoding, const std::string& hrp, const std::vector<uint8_t>& values);

struct DecodeResult
{
    Encoding encoding;         //!< What encoding was detected in the result; Encoding::INVALID if failed.
    std::string hrp;           //!< The human readable part
    std::vector<uint8_t> data; //!< The payload (excluding checksum)

    DecodeResult() : encoding(Encoding::INVALID) {}
    DecodeResult(Encoding enc, std::string&& h, std::vector<uint8_t>&& d) : encoding(enc), hrp(std::move(h)), data(std::move(d)) {}
};

/** Decode a Bech32 or Bech32m string. */
DecodeResult Decode(const std::string& str, CharLimit limit = CharLimit::BECH32);

/** Return the positions of errors in a Bech32 string. */
std::pair<std::string, std::vector<int>> LocateErrors(const std::string& str, CharLimit limit = CharLimit::BECH32);

} // namespace bech32

#endif // BITCOIN_BECH32_H
