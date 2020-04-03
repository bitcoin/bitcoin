// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <vector>
#include <assert.h>
#include <crypto/common.h>

namespace {

constexpr uint32_t INVALID = 0xFFFFFFFF;

uint32_t DecodeBits(std::vector<bool>::const_iterator& bitpos, const std::vector<bool>::const_iterator& endpos, uint8_t minval, const std::vector<uint8_t> &bit_sizes)
{
    uint32_t val = minval;
    bool bit;
    for (std::vector<uint8_t>::const_iterator bit_sizes_it = bit_sizes.begin();
        bit_sizes_it != bit_sizes.end(); ++bit_sizes_it) {
        if (bit_sizes_it + 1 != bit_sizes.end()) {
            if (bitpos == endpos) break;
            bit = *bitpos;
            bitpos++;
        } else {
            bit = 0;
        }
        if (bit) {
            val += (1 << *bit_sizes_it);
        } else {
            for (int b = 0; b < *bit_sizes_it; b++) {
                if (bitpos == endpos) return INVALID; // Reached EOF in mantissa
                bit = *bitpos;
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

const std::vector<uint8_t> TYPE_BIT_SIZES{0, 0, 1};
Instruction DecodeType(std::vector<bool>::const_iterator& bitpos, const std::vector<bool>::const_iterator& endpos)
{
    return Instruction(DecodeBits(bitpos, endpos, 0, TYPE_BIT_SIZES));
}

const std::vector<uint8_t> ASN_BIT_SIZES{15, 16, 17, 18, 19, 20, 21, 22, 23, 24};
uint32_t DecodeASN(std::vector<bool>::const_iterator& bitpos, const std::vector<bool>::const_iterator& endpos)
{
    return DecodeBits(bitpos, endpos, 1, ASN_BIT_SIZES);
}


const std::vector<uint8_t> MATCH_BIT_SIZES{1, 2, 3, 4, 5, 6, 7, 8};
uint32_t DecodeMatch(std::vector<bool>::const_iterator& bitpos, const std::vector<bool>::const_iterator& endpos)
{
    return DecodeBits(bitpos, endpos, 2, MATCH_BIT_SIZES);
}


const std::vector<uint8_t> JUMP_BIT_SIZES{5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30};
uint32_t DecodeJump(std::vector<bool>::const_iterator& bitpos, const std::vector<bool>::const_iterator& endpos)
{
    return DecodeBits(bitpos, endpos, 17, JUMP_BIT_SIZES);
}

}

uint32_t Interpret(const std::vector<bool> &asmap, const std::vector<bool> &ip)
{
    std::vector<bool>::const_iterator pos = asmap.begin();
    const std::vector<bool>::const_iterator endpos = asmap.end();
    uint8_t bits = ip.size();
    uint32_t default_asn = 0;
    uint32_t jump, match, matchlen;
    Instruction opcode;
    while (pos != endpos) {
        opcode = DecodeType(pos, endpos);
        if (opcode == Instruction::RETURN) {
            default_asn = DecodeASN(pos, endpos);
            if (default_asn == INVALID) break; // ASN straddles EOF
            return default_asn;
        } else if (opcode == Instruction::JUMP) {
            jump = DecodeJump(pos, endpos);
            if (jump == INVALID) break; // Jump offset straddles EOF
            if (bits == 0) break;
            if (ip[ip.size() - bits]) {
                if (jump >= endpos - pos) break;
                pos += jump;
            }
            bits--;
        } else if (opcode == Instruction::MATCH) {
            match = DecodeMatch(pos, endpos);
            if (match == INVALID) break; // Match bits straddle EOF
            matchlen = CountBits(match) - 1;
            for (uint32_t bit = 0; bit < matchlen; bit++) {
                if (bits == 0) break;
                if ((ip[ip.size() - bits]) != ((match >> (matchlen - 1 - bit)) & 1)) {
                    return default_asn;
                }
                bits--;
            }
        } else if (opcode == Instruction::DEFAULT) {
            default_asn = DecodeASN(pos, endpos);
            if (default_asn == INVALID) break; // ASN straddles EOF
        } else {
            break; // Instruction straddles EOF
        }
    }
    return 0; // 0 is not a valid ASN
}
