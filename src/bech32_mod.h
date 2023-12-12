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

#ifndef BITCOIN_BECH32_MOD_H
#define BITCOIN_BECH32_MOD_H

#include <chainparams.h>
#include <blsct/double_public_key.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace bech32_mod
{

// double public key after encoding to bech32_mod is 165-byte long consisting of:
// - 2-byte hrp
// - 1-byte separator '1'
// - 154-byte key data (96 bytes / 5 bits = 153.6)
// - 8-byte checksum
constexpr size_t DOUBLE_PUBKEY_ENC_SIZE = 2 + 1 + 154 + 8;

enum class Encoding {
    INVALID, //!< Failed decoding

    BECH32,  //!< Bech32 encoding as defined in BIP173
    BECH32M, //!< Bech32m encoding as defined in BIP350
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
DecodeResult Decode(const std::string& str);

/** Encode DoublePublicKey to Bech32 or Bech32m string. Encoding must be one of BECH32 or BECH32M. */
std::string EncodeDoublePublicKey(
    const CChainParams& params,
    const Encoding encoding,
    const blsct::DoublePublicKey& dpk
);

/** Decode a Bech32 or Bech32m string to concatination of two serialized public keys
 *  Overwrite given data with the concatenated public keys if succeeded. */
bool DecodeDoublePublicKey(
    const CChainParams& params,
    const std::string& str,
    std::vector<uint8_t>& data
);

} // namespace bech32_mod

#endif // BITCOIN_BECH32_MOD_H
