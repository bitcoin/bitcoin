// Copyright (c) 2020 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/fuzz.h>
#include <util/asmap.h>

#include <cstdint>
#include <optional>
#include <vector>

#include <assert.h>

void test_one_input(const std::vector<uint8_t>& buffer)
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
    if (buffer.size() - sep_pos - 1 > 128) return; // At most 128 bits in IP address

    // Checks on asmap
    std::vector<bool> asmap(buffer.begin(), buffer.begin() + sep_pos);
    if (SanityCheckASMap(asmap, buffer.size() - 1 - sep_pos)) {
        // Verify that for valid asmaps, no prefix (except up to 7 zero padding bits) is valid.
        std::vector<bool> asmap_prefix = asmap;
        while (!asmap_prefix.empty() && asmap_prefix.size() + 7 > asmap.size() && asmap_prefix.back() == false) {
            asmap_prefix.pop_back();
        }
        while (!asmap_prefix.empty()) {
            asmap_prefix.pop_back();
            assert(!SanityCheckASMap(asmap_prefix, buffer.size() - 1 - sep_pos));
        }
        // No address input should trigger assertions in interpreter
        std::vector<bool> addr(buffer.begin() + sep_pos + 1, buffer.end());
        (void)Interpret(asmap, addr);
    }
}
