// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <vector>
#include <assert.h>
#include <crypto/common.h>

namespace {

uint32_t DecodeBits(std::vector<bool>::const_iterator& bitpos, uint8_t minval, const std::vector<uint8_t> &bit_sizes)
{
    uint32_t val = minval;
    bool bit;
    for (std::vector<uint8_t>::const_iterator bit_sizes_it = bit_sizes.begin();
        bit_sizes_it != bit_sizes.end(); ++bit_sizes_it) {
        if (bit_sizes_it + 1 != bit_sizes.end()) {
            bit = *bitpos;
            bitpos++;
        } else {
            bit = 0;
        }
        if (bit) {
            val += (1 << *bit_sizes_it);
        } else {
            for (int b = 0; b < *bit_sizes_it; b++) {
                bit = *bitpos;
                bitpos++;
                val += bit << (*bit_sizes_it - 1 - b);
            }
            return val;
        }
    }
    return -1;
}

const std::vector<uint8_t> TYPE_BIT_SIZES{0, 0, 1};
uint32_t DecodeType(std::vector<bool>::const_iterator& bitpos)
{
    return DecodeBits(bitpos, 0, TYPE_BIT_SIZES);
}

const std::vector<uint8_t> ASN_BIT_SIZES{15, 16, 17, 18, 19, 20, 21, 22, 23, 24};
uint32_t DecodeASN(std::vector<bool>::const_iterator& bitpos)
{
    return DecodeBits(bitpos, 1, ASN_BIT_SIZES);
}


const std::vector<uint8_t> MATCH_BIT_SIZES{1, 2, 3, 4, 5, 6, 7, 8};
uint32_t DecodeMatch(std::vector<bool>::const_iterator& bitpos)
{
    return DecodeBits(bitpos, 2, MATCH_BIT_SIZES);
}


const std::vector<uint8_t> JUMP_BIT_SIZES{5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30};
uint32_t DecodeJump(std::vector<bool>::const_iterator& bitpos)
{
    return DecodeBits(bitpos, 17, JUMP_BIT_SIZES);
}

}

uint32_t Interpret(const std::vector<bool> &asmap, const std::vector<bool> &ip)
{
    std::vector<bool>::const_iterator pos = asmap.begin();
    uint8_t bits = ip.size();
    uint8_t default_asn = 0;
    uint32_t opcode, jump, match, matchlen;
    while (1) {
        assert(pos != asmap.end());
        opcode = DecodeType(pos);
        if (opcode == 0) {
            return DecodeASN(pos);
        } else if (opcode == 1) {
            jump = DecodeJump(pos);
            if (ip[ip.size() - bits]) {
                pos += jump;
            }
            bits--;
        } else if (opcode == 2) {
            match = DecodeMatch(pos);
            matchlen = CountBits(match) - 1;
            for (uint32_t bit = 0; bit < matchlen; bit++) {
                if ((ip[ip.size() - bits]) != ((match >> (matchlen - 1 - bit)) & 1)) {
                    return default_asn;
                }
                bits--;
            }
        } else if (opcode == 3) {
            default_asn = DecodeASN(pos);
        } else {
            assert(0);
        }
    }
}
