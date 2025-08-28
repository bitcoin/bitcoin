// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// codex32 is a string encoding format for BIP-32 seeds. Like bech32 and
// bech32m, the outputs consist of a human-readable part (alphanumeric),
// a separator character (1), and a base32 data section. The final 13
// characters are a checksum.
//
// For more information, see BIP 93.

#ifndef BITCOIN_CODEX32_H
#define BITCOIN_CODEX32_H

#include <stdint.h>
#include <array>
#include <cassert>
#include <string>
#include <vector>

#include <bech32.h>
#include <util/strencodings.h>

namespace codex32
{

enum Error {
    OK,
    BAD_CHECKSUM,
    BECH32_DECODE,
    INVALID_HRP,
    INVALID_ID_LEN,
    INVALID_ID_CHAR,
    INVALID_LENGTH,
    INVALID_K,
    INVALID_SHARE_IDX,
    TOO_FEW_SHARES,
    DUPLICATE_SHARE,
    MISMATCH_K,
    MISMATCH_ID,
    MISMATCH_LENGTH,
};

std::string ErrorString(Error e);

class Result
{
public:
    /** Construct a codex32 result by parsing a string */
    Result(const std::string& str);

    /** Construct a codex32 directly from a HRP, k, seed ID, share index and payload
     *
     * This constructor requires the hrp to be the lowercase string "ms", but will
     * ignore the case of `id` and `share_idx`. */
    Result(std::string&& hrp, size_t k, const std::string& id, char share_idx, const std::vector<unsigned char>& data);

    /** Construct a codex32 result by interpolating a set of input shares to obtain an output share
     *
     * Requires that all input shares have the same k and seed ID */
    Result(const std::vector<Result>& shares, char output_idx);

    /** Boolean indicating whether the data was successfully parsed.
     *
     * If this returns false, most of the other methods on this class will assert. */
    bool IsValid() const {
        return m_valid == OK;
    }

    /** Accessor for the specific parsing/construction error */
    Error error() const {
        return m_valid;
    }

    /** Accessor for the human-readable part of the codex32 string */
    const std::string& GetHrp() const {
        assert(IsValid());
        return m_hrp;
    }

    /** Accessor for the seed ID, in string form */
    std::string GetIdString() const;

    /** Accessor for the secret sharing threshold; 0 for a bare seed; (size_t)-1 if unavailable/invalid */
    size_t GetK() const;

    /** Accessor for the share index; (uint8_t)-1 if unavailable/invalid */
    char GetShareIndex() const {
        assert(IsValid());
        return bech32::internal::CHARSET[m_data[5]];
    }

    /** Accessor for the binary payload data (in base 256, not gf32) */
    std::vector<unsigned char> GetPayload() const {
        assert(IsValid());

        std::vector<unsigned char> ret;
        ret.reserve(((m_data.size() - 6) * 5) / 8);
        // Note that `ConvertBits` returns a bool indicating whether or not nonzero bits
        // were discarded. In BIP 93, we discard bits regardless of whether they are 0,
        // so this is not an error and does not need to be checked.
        ConvertBits<5, 8, false>([&](unsigned char c) { ret.push_back(c); }, m_data.begin() + 6, m_data.end());
        return ret;
    };

    /** (Re-)encode the codex32 data as a hrp string */
    std::string Encode() const;

private:
    Error m_valid;               //!< codex32::OK if the string was decoded correctly

    std::string m_hrp;           //!< The human readable part
    std::vector<uint8_t> m_data; //!< The payload (remaining data, excluding checksum)
};

} // namespace codex32

#endif // BITCOIN_CODEX32_H
