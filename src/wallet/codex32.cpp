// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/* Implementation of BIP-93 "codex32: Checksummed SSSS-aware BIP32 seeds".
 *
 * There are two representations, short and long:
 *
 *   CODEX32 := HRP "1" SHORT-DATA | LONG-DATA
 *   HRP := "ms" | "MS"
 *   SHORT-DATA := THRESHOLD IDENTIFIER SHAREINDEX SHORT-PAYLOAD SHORT-CHECKSUM
 *   LONG-DATA := THRESHOLD IDENTIFIER SHAREINDEX LONG-PAYLOAD LONG-CHECKSUM
 *
 *   THRESHOLD = "0" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
 *   IDENTIFER := BECH32*4
 *   SHAREINDEX := BECH32
 *
 *   SHORT-PAYLOAD := BECH32 [0 - 74 times]
 *   SHORT-CHECKSUM := BECH32*13
 *
 *   LONG-PAYLOAD := BECH32 [75 - 103 times]
 *   LONG-CHECKSUM := BECH32*15
 *
 * Thus, a short codex32 string has 22 bytes of non-payload, so 22 to 96 characters long.
 * A long codex32 string has 24 bytes of non-payload, so 99 to 127 characters.
 */

#include <wallet/codex32.h>
#include <bech32.h> // Bitcoin Core's bech32 implementation
#include <util/strencodings.h> // For IsSpace, ToLower
#include <algorithm>
#include <cassert>
#include <cstring>
#include <optional>

namespace {

struct ChecksumEngine {
    std::array<uint8_t, 15> generator;
    std::array<uint8_t, 15> residue;
    std::array<uint8_t, 15> target;
    size_t len;
    size_t max_payload_len;
};

static const std::array<ChecksumEngine, 2> INITIAL_ENGINE_CSUM = {{
    /* Short Codex32 Engine */
    {
        {25, 27, 17, 8, 0, 25, 25, 25, 31, 27, 24, 16, 16, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
        {16, 25, 24, 3, 25, 11, 16, 23, 29, 3, 25, 17, 10, 0, 0},
        13,
        74,
    },
    /* Long Codex32 Engine */
    {
        {15, 10, 25, 26, 9, 25, 21, 6, 23, 21, 6, 5, 22, 4, 23},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {16, 25, 24, 3, 25, 11, 16, 23, 29, 3, 25, 17, 10, 25, 6},
        15,
        103,
    }
}};

static const uint8_t LOGI[32] = {
     0,  0,  1, 14,  2, 28, 15, 22,
     3,  5, 29, 26, 16,  7, 23, 11,
     4, 25,  6, 10, 30, 13, 27, 21,
    17, 18,  8, 19, 24,  9, 12, 20,
};

static const uint8_t LOG_INV[31] = {
     1,  2,  4,  8, 16,  9, 18, 13,
    26, 29, 19, 15, 30, 21,  3,  6,
    12, 24, 25, 27, 31, 23,  7, 14,
    28, 17, 11, 22,  5, 10, 20,
};

inline void AdditionGF32(uint8_t& x, uint8_t y)
{
    x = x ^ y;
}

inline void MultiplyGF32(uint8_t& x, uint8_t y)
{
    if (x == 0 || y == 0) {
        x = 0;
    } else {
        x = LOG_INV[(LOGI[x] + LOGI[y]) % 31];
    }
}

/* Helper to input a single field element in the checksum engine. */
void InputFE(const std::array<uint8_t, 15>& generator, std::array<uint8_t, 15>& residue, uint8_t e, size_t len)
{
    uint8_t xn = residue[0];

    for (size_t i = 1; i < len; i++) {
        residue[i - 1] = residue[i];
    }

    residue[len - 1] = e;

    for (size_t i = 0; i < len; i++) {
        uint8_t x = generator[i];
        MultiplyGF32(x, xn);
        AdditionGF32(residue[i], x);
    }
}

/* Helper to input the HRP of codex32 string into the checksum engine */
void InputHRP(const std::array<uint8_t, 15>& generator, std::array<uint8_t, 15>& residue, const std::string& hrp, size_t len)
{
    size_t i = 0;
    for (i = 0; i < hrp.length(); i++) {
        InputFE(generator, residue, hrp[i] >> 5, len);
    }
    InputFE(generator, residue, hrp[i] >> 0, len);
    for (i = 0; i < hrp.length(); i++) {
        InputFE(generator, residue, hrp[i] & 0x1f, len);
    }
}

/* Helper to input data string of codex32 into the checksum engine. */
void InputDataStr(const std::array<uint8_t, 15>& generator, std::array<uint8_t, 15>& residue, const std::string& datastr, size_t len)
{
    for (size_t i = 0; i < datastr.length(); i++) {
        InputFE(generator, residue, bech32::CHARSET_REV[(int)datastr[i]], len);
    }
}

void InputOwnTarget(const std::array<uint8_t, 15>& generator, std::array<uint8_t, 15>& residue, const std::array<uint8_t, 15>& target, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        InputFE(generator, residue, target[i], len);
    }
}

/* Helper to verify codex32 checksum */
bool ChecksumVerify(const std::string& hrp, const std::string& codex_datastr, const ChecksumEngine& initial_engine)
{
    ChecksumEngine engine = initial_engine;

    InputHRP(engine.generator, engine.residue, hrp, engine.len);
    InputDataStr(engine.generator, engine.residue, codex_datastr, engine.len);

    return std::memcmp(engine.target.data(), engine.residue.data(), engine.len) == 0;
}

void CalculateChecksum(const std::string& hrp, std::string& csum, const std::string& codex_datastr, const ChecksumEngine& initial_engine)
{
    ChecksumEngine engine = initial_engine;

    InputHRP(engine.generator, engine.residue, hrp, engine.len);
    InputDataStr(engine.generator, engine.residue, codex_datastr, engine.len);
    InputOwnTarget(engine.generator, engine.residue, engine.target, engine.len);

    csum.clear();
    for (size_t i = 0; i < engine.len; i++) {
        csum += bech32::CHARSET[engine.residue[i]];
    }
}

/* Helper to fetch data from payload as a valid hex buffer */
std::vector<uint8_t> DecodePayload(const std::string& payload)
{
    std::vector<uint8_t> ret;
    ret.reserve((payload.length() * 5 + 7) / 8);

    uint8_t next_byte = 0;
    uint8_t rem = 0;

    /* We have already checked this is a valid bech32 string! */
    for (size_t i = 0; i < payload.length(); i++) {
        int ch = payload[i];
        uint8_t fe = bech32::CHARSET_REV[ch];

        if (rem < 3) {
            // If we are within 3 bits of the start we can fit the whole next char in
            next_byte |= fe << (3 - rem);
        }
        else if (rem == 3) {
            // If we are exactly 3 bits from the start then this char fills in the byte
            ret.push_back(next_byte | fe);
            next_byte = 0;
        }
        else { // rem > 3
            // Otherwise we have to break it in two
            uint8_t overshoot = rem - 3;
            assert(overshoot > 0);
            ret.push_back(next_byte | (fe >> overshoot));
            next_byte = (fe << (8 - overshoot)) & 0xFF;
        }

        rem = (rem + 5) % 8;
    }

    /* BIP-93:
     * Any incomplete group at the end MUST be 4 bits or less, and is discarded.
     */
    if (rem > 4) {
        return {};
    }

    return ret;
}

/* Checks case inconsistency, and for non-bech32 chars. */
std::optional<std::pair<std::string, size_t>> Bech32CaseFixup(const std::string& codex32str)
{
    std::string result = codex32str;
    size_t sep_pos = std::string::npos;
    bool was_upper = false;

    // If first is upper, we'll lowercase everything
    if (!result.empty() && std::isupper(result[0])) {
        was_upper = true;
    }

    for (size_t i = 0; i < result.length(); i++) {
        int c = result[i];

        if (c == '1') {
            // Two separators?
            if (sep_pos != std::string::npos) {
                return std::nullopt;
            }
            sep_pos = i;
            continue;
        }

        if (c < 0 || c > 127) {
            return std::nullopt;
        }

        if (was_upper) {
            // Mixed case not allowed!
            if (std::islower(c)) {
                return std::nullopt;
            }
            result[i] = std::tolower(c);
            c = result[i];
        } else {
            if (std::isupper(c)) {
                return std::nullopt;
            }
        }

        if (bech32::CHARSET_REV[c] == -1) {
            return std::nullopt;
        }
    }

    return std::make_pair(result, sep_pos);
}

} // namespace

/* Return nullopt if the codex32 is invalid */
std::optional<Codex32> codex32_decode(const std::string& hrp,
                                     const std::string& codex32str,
                                     std::string& error_str)
{
    Codex32 parts;

    // Lowercase it all, iff it's all uppercase.
    auto fixup_result = Bech32CaseFixup(codex32str);
    if (!fixup_result) {
        error_str = "Not a valid bech32 string!";
        return std::nullopt;
    }

    const std::string& normalized_str = fixup_result->first;
    size_t sep_pos = fixup_result->second;

    if (sep_pos == std::string::npos) {
        error_str = "Separator doesn't exist!";
        return std::nullopt;
    }

    parts.hrp = normalized_str.substr(0, sep_pos);
    if (!hrp.empty() && parts.hrp != hrp) {
        error_str = "Invalid hrp " + parts.hrp + "!";
        return std::nullopt;
    }

    std::string codex_datastr = normalized_str.substr(sep_pos + 1);
    size_t maxlen = codex_datastr.length();

    // If it's short, use short checksum engine. If it's invalid,
    // use short checksum and we'll fail when payload is too long.
    const ChecksumEngine& csum_engine = INITIAL_ENGINE_CSUM[maxlen >= 96 ? 1 : 0];
    if (!ChecksumVerify(parts.hrp, codex_datastr, csum_engine)) {
        error_str = "Invalid checksum!";
        return std::nullopt;
    }

    // Check minimum length
    if (codex_datastr.length() < 1 + 4 + 1 + csum_engine.len) {
        error_str = "Too short!";
        return std::nullopt;
    }

    // Pull fixed parts
    char threshold_char = codex_datastr[0];
    std::memcpy(parts.id.data(), codex_datastr.data() + 1, 4);
    parts.id[4] = '\0';
    parts.share_idx = codex_datastr[5];

    // Calculate payload length (total - threshold - id - share_idx - checksum)
    size_t payload_len = codex_datastr.length() - 6 - csum_engine.len;
    if (payload_len > csum_engine.max_payload_len) {
        error_str = "Invalid length!";
        return std::nullopt;
    }

    // Extract payload
    std::string payload_str = codex_datastr.substr(6, payload_len);
    parts.payload = DecodePayload(payload_str);
    if (parts.payload.empty() && !payload_str.empty()) {
        error_str = "Invalid payload!";
        return std::nullopt;
    }

    if (parts.share_idx == 's') {
        parts.type = Codex32Encoding::SECRET;
    } else {
        parts.type = Codex32Encoding::SHARE;
    }

    parts.threshold = threshold_char - '0';
    if (parts.threshold > 9 || parts.threshold < 2) {
        // Can't be 1 because bech32 '1' is separator
        if (parts.threshold != 0) {
            error_str = "Invalid threshold!";
            return std::nullopt;
        }
    }

    if (parts.threshold == 0 && parts.type != Codex32Encoding::SECRET) {
        error_str = "Expected share index s for threshold 0!";
        return std::nullopt;
    }

    return parts;
}

/* Returns Codex32 encoded secret of the seed provided. */
std::string codex32_secret_encode(const std::string& hrp,
                                 const std::string& id,
                                 uint32_t threshold,
                                 const std::vector<uint8_t>& seed,
                                 std::string& error_str)
{
    // FIXME: Our code assumes a two-letter HRP! Larger won't allow a
    // 128-bit secret in a "standard billfold metal wallet" according to
    // Russell O'Connor
    assert(hrp.length() == 2);

    if (threshold > 9 || threshold < 2) {
        if (threshold != 0) {
            error_str = "Invalid threshold " + std::to_string(threshold);
            return "";
        }
    }

    if (id.length() != 4) {
        error_str = "Invalid id: must be 4 characters";
        return "";
    }

    for (size_t i = 0; i < id.length(); i++) {
        if (id[i] & 0x80) {
            error_str = "Invalid id: must be ASCII";
            return "";
        }

        int8_t rev = bech32::CHARSET_REV[(int)id[i]];
        if (rev == -1) {
            error_str = "Invalid id: must be valid bech32 string";
            return "";
        }
        if (bech32::CHARSET[rev] != id[i]) {
            error_str = "Invalid id: must be lower-case";
            return "";
        }
    }

    // Every codex32 has hrp `ms` and since we are generating a
    // secret it's share index would be `s` and threshold given by user.
    std::string bip93 = hrp + "1" + std::to_string(threshold) + id + "s";

    uint8_t next_u5 = 0, rem = 0;

    for (size_t i = 0; i < seed.size(); i++) {
        // Each byte provides at least one u5. Push that.
        uint8_t u5 = ((next_u5 << (5 - rem)) | (seed[i] >> (3 + rem))) & 0x1F;

        bip93 += bech32::CHARSET[u5];
        next_u5 = seed[i] & ((1 << (3 + rem)) - 1);

        // If there were 2 or more bits from the last iteration, then
        // this iteration will push *two* u5s.
        if (rem >= 2) {
            bip93 += bech32::CHARSET[next_u5 >> (rem - 2)];
            next_u5 &= (1 << (rem - 2)) - 1;
        }
        rem = (rem + 8) % 5;
    }
    if (rem > 0) {
        bip93 += bech32::CHARSET[(next_u5 << (5 - rem)) & 0x1F];
    }

    const ChecksumEngine& csum_engine = INITIAL_ENGINE_CSUM[seed.size() >= 51 ? 1 : 0];
    std::string csum;
    CalculateChecksum(hrp, csum, bip93.substr(3), csum_engine);
    bip93 += csum;

    return bip93;
}
