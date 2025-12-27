// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netaddress.h>
#include <util/asmap.h>
#include <test/fuzz/fuzz.h>

#include <cstdint>
#include <optional>
#include <vector>

#include <cassert>

std::vector<std::byte> BitsToBytes(std::span<const uint8_t> bits) noexcept
{
    std::vector<std::byte> ret;
    uint8_t next_byte{0};
    int next_byte_bits{0};
    for (uint8_t val : bits) {
        next_byte |= (val & 1) << (next_byte_bits++);
        if (next_byte_bits == 8) {
            ret.push_back(std::byte(next_byte));
            next_byte = 0;
            next_byte_bits = 0;
        }
    }
    if (next_byte_bits) ret.push_back(std::byte(next_byte));

    return ret;
}

FUZZ_TARGET(asmap_direct)
{
    // Encoding: [asmap using 1 bit / byte] 0xFF [addr using 1 bit / byte]
    std::optional<size_t> sep_pos_opt;
    for (size_t pos = 0; pos < buffer.size(); ++pos) {
        uint8_t x = buffer[pos];
        if ((x & 0xFE) == 0) continue;
        if (x == 0xFF) {
            if (sep_pos_opt) return;
            sep_pos_opt = pos;
        } else {
            return;
        }
    }
    if (!sep_pos_opt) return; // Needs exactly 1 separator
    const size_t sep_pos{sep_pos_opt.value()};
    const size_t ip_len{buffer.size() - sep_pos - 1};
    if (ip_len > 128) return; // At most 128 bits in IP address

    // Checks on asmap
    auto asmap = BitsToBytes(buffer.first(sep_pos));
    if (SanityCheckAsmap(asmap, ip_len)) {
        // Verify that for valid asmaps, no prefix (except up to 7 zero padding bits) is valid.
        for (size_t prefix_len = sep_pos - 1; prefix_len > 0; --prefix_len) {
            auto prefix = BitsToBytes(buffer.first(prefix_len));
            // We have to skip the prefixes of the same length as the original
            // asmap, since they will contain some zero padding bits in the last
            // byte.
            if (prefix.size() == asmap.size()) continue;
            assert(!SanityCheckAsmap(prefix, ip_len));
        }

        // No address input should trigger assertions in interpreter
        auto addr = BitsToBytes(buffer.subspan(sep_pos + 1));
        (void)Interpret(asmap, addr);
    }
}
