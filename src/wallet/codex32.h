// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_CODEX32_H
#define BITCOIN_WALLET_CODEX32_H

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <optional>

/* Supported encodings. */
enum class Codex32Encoding {
   SHARE,
   SECRET
};

/* Decoded codex32 parts */
struct Codex32 {
    /* "ms" */
    std::string hrp;
    /* 0, or 2-9 */
    uint8_t threshold;
    /* Four valid bech32 characters which identify this complete codex32 secret */
    std::array<char, 5> id; // 4 chars + null terminator
    /* Valid bech32 character identifying this share of the secret, or `s` for unshared */
    char share_idx;
    /* The actual data payload */
    std::vector<uint8_t> payload;
    /* Is this a share, or a secret? */
    Codex32Encoding type;
};

/** Decode a codex32 or codex32l string
 *
 *  Out: error_str:   Will be set with the reason of failure if decoding fails
 *  In:  hrp:         If non-empty, a hrp which must match.
 *       codex32str:  codex32 string to decode.
 *
 *  Returns decoded Codex32 struct if successful, std::nullopt if decoding failed
 */
std::optional<Codex32> codex32_decode(const std::string& hrp,
                                     const std::string& codex32str,
                                     std::string& error_str);

/** Encode a seed into codex32 secret format.
 *
 *  In:  hrp:        2 character human-readable-prefix
 *       id:         Valid 4 char string identifying the secret
 *       threshold:  Threshold according to the bip93
 *       seed:       The secret data
 *
 *  Returns the seed encoded in bip93 format, or empty string on error with error_str set
 */
std::string codex32_secret_encode(const std::string& hrp,
                                 const std::string& id,
                                 uint32_t threshold,
                                 const std::vector<uint8_t>& seed,
                                 std::string& error_str);

#endif // BITCOIN_WALLET_CODEX32_H
