// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/asmap.h>

#include <clientversion.h>
#include <hash.h>
#include <logging.h>
#include <serialize.h>
#include <streams.h>
#include <uint256.h>
#include <util/fs.h>

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <span>
#include <utility>
#include <vector>

namespace {

constexpr uint32_t INVALID = 0xFFFFFFFF;

inline bool GetBitLE(std::span<const std::byte> bytes, uint32_t bitpos) noexcept
{
    return (std::to_integer<uint8_t>(bytes[bitpos / 8]) >> (bitpos % 8)) & 1;
}

inline bool GetBitBE(std::span<const std::byte> bytes, uint32_t bitpos) noexcept
{
    return (std::to_integer<uint8_t>(bytes[(bytes.size() * 8 - bitpos) / 8]) >> (7 - ((bytes.size() * 8 - bitpos) % 8))) & 1;
}

uint32_t DecodeBits(size_t& bitpos, const std::span<const std::byte>& data, uint8_t minval, const std::span<const uint8_t> bit_sizes)
{
    uint32_t val = minval;
    bool bit;
    for (auto bit_sizes_it = bit_sizes.begin(); bit_sizes_it != bit_sizes.end(); ++bit_sizes_it) {
        if (bit_sizes_it + 1 != bit_sizes.end()) {
            if (bitpos >= data.size() * 8) break;
            bit = GetBitLE(data, bitpos);
            bitpos++;
        } else {
            bit = 0;
        }
        if (bit) {
            val += (1 << *bit_sizes_it);
        } else {
            for (int b = 0; b < *bit_sizes_it; b++) {
                if (bitpos >= data.size() * 8) return INVALID; // Reached EOF in mantissa
                bit = GetBitLE(data, bitpos);
                bitpos++;
                val += bit << (*bit_sizes_it - 1 - b);
            }
            return val;
        }
    }
    return INVALID; // Reached EOF in exponent
}

enum class Instruction : uint32_t
{
    RETURN = 0,
    JUMP = 1,
    MATCH = 2,
    DEFAULT = 3,
};

constexpr uint8_t TYPE_BIT_SIZES[]{0, 0, 1};
Instruction DecodeType(size_t& bitpos, const std::span<const std::byte>& data)
{
    return Instruction(DecodeBits(bitpos, data, 0, TYPE_BIT_SIZES));
}

constexpr uint8_t ASN_BIT_SIZES[]{15, 16, 17, 18, 19, 20, 21, 22, 23, 24};
uint32_t DecodeASN(size_t& bitpos, const std::span<const std::byte>& data)
{
    return DecodeBits(bitpos, data, 1, ASN_BIT_SIZES);
}

constexpr uint8_t MATCH_BIT_SIZES[]{1, 2, 3, 4, 5, 6, 7, 8};
uint32_t DecodeMatch(size_t& bitpos, const std::span<const std::byte>& data)
{
    return DecodeBits(bitpos, data, 2, MATCH_BIT_SIZES);
}

constexpr uint8_t JUMP_BIT_SIZES[]{5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30};
uint32_t DecodeJump(size_t& bitpos, const std::span<const std::byte>& data)
{
    return DecodeBits(bitpos, data, 17, JUMP_BIT_SIZES);
}

}

uint32_t Interpret(const std::span<const std::byte> asmap, const std::span<const std::byte> ip)
{
    size_t pos{0};
    const size_t endpos{asmap.size() * 8};
    uint8_t bits = ip.size() * 8;
    uint32_t default_asn = 0;
    uint32_t jump, match, matchlen;
    Instruction opcode;
    while (pos < endpos) {
        opcode = DecodeType(pos, asmap);
        if (opcode == Instruction::RETURN) {
            default_asn = DecodeASN(pos, asmap);
            if (default_asn == INVALID) break; // ASN straddles EOF
            return default_asn;
        } else if (opcode == Instruction::JUMP) {
            jump = DecodeJump(pos, asmap);
            if (jump == INVALID) break; // Jump offset straddles EOF
            if (bits == 0) break; // No input bits left
            if (int64_t{jump} >= static_cast<int64_t>(endpos - pos)) break; // Jumping past EOF
            if (GetBitBE(ip, bits)) {
                pos += jump;
            }
            bits--;
        } else if (opcode == Instruction::MATCH) {
            match = DecodeMatch(pos, asmap);
            if (match == INVALID) break; // Match bits straddle EOF
            matchlen = std::bit_width(match) - 1;
            if (bits < matchlen) break; // Not enough input bits
            for (uint32_t bit = 0; bit < matchlen; bit++) {
                if (GetBitBE(ip, bits) != ((match >> (matchlen - 1 - bit)) & 1)) {
                    return default_asn;
                }
                bits--;
            }
        } else if (opcode == Instruction::DEFAULT) {
            default_asn = DecodeASN(pos, asmap);
            if (default_asn == INVALID) break; // ASN straddles EOF
        } else {
            break; // Instruction straddles EOF
        }
    }
    assert(false); // Reached EOF without RETURN, or aborted (see any of the breaks above) - should have been caught by SanityCheckASMap below
    return 0; // 0 is not a valid ASN
}

bool SanityCheckASMap(const std::span<const std::byte> asmap, int bits)
{
    size_t pos{0};
    const size_t endpos{asmap.size() * 8};
    std::vector<std::pair<uint32_t, int>> jumps; // All future positions we may jump to (bit offset in asmap -> bits to consume left)
    jumps.reserve(bits);
    Instruction prevopcode = Instruction::JUMP;
    bool had_incomplete_match = false;
    while (pos != endpos) {
        if (!jumps.empty() && pos >= jumps.back().first) return false; // There was a jump into the middle of the previous instruction
        Instruction opcode = DecodeType(pos, asmap);
        if (opcode == Instruction::RETURN) {
            if (prevopcode == Instruction::DEFAULT) return false; // There should not be any RETURN immediately after a DEFAULT (could be combined into just RETURN)
            uint32_t asn = DecodeASN(pos, asmap);
            if (asn == INVALID) return false; // ASN straddles EOF
            if (jumps.empty()) {
                // Nothing to execute anymore
                if (endpos - pos > 7) return false; // Excessive padding
                while (pos != endpos) {
                    if (GetBitLE(asmap, pos)) return false; // Nonzero padding bit
                    ++pos;
                }
                return true; // Sanely reached EOF
            } else {
                // Continue by pretending we jumped to the next instruction
                if (pos != jumps.back().first) return false; // Unreachable code
                bits = jumps.back().second; // Restore the number of bits we would have had left after this jump
                jumps.pop_back();
                prevopcode = Instruction::JUMP;
            }
        } else if (opcode == Instruction::JUMP) {
            uint32_t jump = DecodeJump(pos, asmap);
            if (jump == INVALID) return false; // Jump offset straddles EOF
            if (int64_t{jump} > static_cast<int64_t>(endpos - pos)) return false; // Jump out of range
            if (bits == 0) return false; // Consuming bits past the end of the input
            --bits;
            uint32_t jump_offset = pos + jump;
            if (!jumps.empty() && jump_offset >= jumps.back().first) return false; // Intersecting jumps
            jumps.emplace_back(jump_offset, bits);
            prevopcode = Instruction::JUMP;
        } else if (opcode == Instruction::MATCH) {
            uint32_t match = DecodeMatch(pos, asmap);
            if (match == INVALID) return false; // Match bits straddle EOF
            int matchlen = std::bit_width(match) - 1;
            if (prevopcode != Instruction::MATCH) had_incomplete_match = false;
            if (matchlen < 8 && had_incomplete_match) return false; // Within a sequence of matches only at most one should be incomplete
            had_incomplete_match = (matchlen < 8);
            if (bits < matchlen) return false; // Consuming bits past the end of the input
            bits -= matchlen;
            prevopcode = Instruction::MATCH;
        } else if (opcode == Instruction::DEFAULT) {
            if (prevopcode == Instruction::DEFAULT) return false; // There should not be two successive DEFAULTs (they could be combined into one)
            uint32_t asn = DecodeASN(pos, asmap);
            if (asn == INVALID) return false; // ASN straddles EOF
            prevopcode = Instruction::DEFAULT;
        } else {
            return false; // Instruction straddles EOF
        }
    }
    return false; // Reached EOF without RETURN instruction
}

bool CheckAsmap(const std::span<const std::byte> data)
{
    if (!SanityCheckASMap(data, 128)) {
        LogWarning("Sanity check of asmap data failed\n");
        return false;
    }
    return true;
}

std::vector<std::byte> DecodeAsmap(fs::path path)
{
    FILE *filestr = fsbridge::fopen(path, "rb");
    AutoFile file{filestr};
    if (file.IsNull()) {
        LogWarning("Failed to open asmap file from disk");
        return {};
    }
    int64_t length{file.size()};
    LogInfo("Opened asmap file %s (%d bytes) from disk", fs::quoted(fs::PathToString(path)), length);
    std::vector<std::byte> buffer(length);
    file.read(buffer);

    if (!CheckAsmap(buffer)) {
        LogWarning("Sanity check of asmap file %s failed", fs::quoted(fs::PathToString(path)));
        return {};
    }

    return buffer;
}

uint256 AsmapVersion(const std::span<const std::byte> data)
{
    if (data.empty()) return {};

    HashWriter asmap_hasher;
    asmap_hasher << data;
    return asmap_hasher.GetHash();
}
