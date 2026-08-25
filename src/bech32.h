// Copyright (c) 2017, 2021 Pieter Wuille
// Copyright (c) 2021-present The Bitcoin Core developers
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

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace bech32
{

inline constexpr size_t CHECKSUM_SIZE = 6;
inline constexpr char SEPARATOR = '1';

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

/** Error codes describing how a Bech32(m) string is invalid, as detected by LocateErrors. */
enum class Error {
    NONE,                        //!< No error
    TOO_LONG,                    //!< String exceeds the character limit
    INVALID_CHARS_OR_MIXED_CASE, //!< Invalid character(s), or mixed uppercase and lowercase
    MISSING_SEPARATOR,           //!< No separator character present
    INVALID_SEPARATOR_POSITION,  //!< Separator at the start, or too close to the end
    INVALID_BASE32_CHAR,         //!< Character in the data section not in the Bech32 character set
    INVALID_CHECKSUM,            //!< Checksum invalid under both Bech32 and Bech32m rules
    INVALID_BECH32_CHECKSUM,     //!< Checksum invalid, fewest errors found under Bech32 rules
    INVALID_BECH32M_CHECKSUM,    //!< Checksum invalid, fewest errors found under Bech32m rules
};

struct LocateErrorsResult {
    Error error;                //!< The detected error; Error::NONE if there is none.
    std::vector<int> locations; //!< Character positions of the error(s); may be empty.
};

/** Classify and locate errors in a Bech32(m) string. The caller decides how to present the error. */
LocateErrorsResult LocateErrors(const std::string& str, CharLimit limit = CharLimit::BECH32);

} // namespace bech32

#endif // BITCOIN_BECH32_H
