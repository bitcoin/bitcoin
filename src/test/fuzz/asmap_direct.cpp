// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netaddress.h>
#include <util/asmap.h>
#include <test/fuzz/fuzz.h>

#include <cstdint>
#include <optional>
#include <vector>

#include <assert.h>

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
    if (buffer.size() - sep_pos - 1 > 128) return; // At most 128 bits in IP address

    // Checks on asmap
    std::vector<std::byte> asmap(reinterpret_cast<const std::byte*>(buffer.data()),
                                 reinterpret_cast<const std::byte*>(buffer.data() + sep_pos));
    if (SanityCheckASMap(std::span<const std::byte>(asmap), buffer.size() - 1 - sep_pos)) {
        // Verify that for valid asmaps, no prefix (except up to 7 zero padding bits) is valid.
        std::vector<std::byte> asmap_prefix = asmap;
        while (!asmap_prefix.empty() && asmap_prefix.size() + 7 > asmap.size() && asmap_prefix.back() == std::byte{0}) {
            asmap_prefix.pop_back();
        }
        while (!asmap_prefix.empty()) {
            asmap_prefix.pop_back();
            assert(!SanityCheckASMap(std::span<const std::byte>(asmap_prefix), buffer.size() - 1 - sep_pos));
        }
        // No address input should trigger assertions in interpreter
        std::vector<std::byte> addr(reinterpret_cast<const std::byte*>(buffer.data() + sep_pos + 1),
                                    reinterpret_cast<const std::byte*>(buffer.data() + buffer.size()));
        (void)Interpret(std::span<const std::byte>(asmap), std::span<const std::byte>(addr));
    }
}
