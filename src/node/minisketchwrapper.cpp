// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/minisketchwrapper.h>

#include <logging.h>
#include <util/time.h>

#include <minisketch.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace node {
namespace {

static constexpr uint32_t BITS = 32;

uint32_t FindBestImplementation()
{
    std::optional<std::pair<SteadyClock::duration, uint32_t>> best;

    uint32_t max_impl = Minisketch::MaxImplementation();
    for (uint32_t impl = 0; impl <= max_impl; ++impl) {
        std::vector<SteadyClock::duration> benches;
        uint64_t offset = 0;
        /* Run a little benchmark with capacity 32, adding 184 entries, and decoding 11 of them once. */
        for (int b = 0; b < 11; ++b) {
            if (!Minisketch::ImplementationSupported(BITS, impl)) break;
            Minisketch sketch(BITS, impl, 32);
            auto start = SteadyClock::now();
            for (uint64_t e = 0; e < 100; ++e) {
                sketch.Add(e*1337 + b*13337 + offset);
            }
            for (uint64_t e = 0; e < 84; ++e) {
                sketch.Add(e*1337 + b*13337 + offset);
            }
            offset += (*sketch.Decode(32))[0];
            auto stop = SteadyClock::now();
            benches.push_back(stop - start);
        }
        /* Remember which implementation has the best median benchmark time. */
        if (!benches.empty()) {
            std::sort(benches.begin(), benches.end());
            if (!best || best->first > benches[5]) {
                best = std::make_pair(benches[5], impl);
            }
        }
    }
    assert(best.has_value());
    LogInfo("Using Minisketch implementation number %i", best->second);
    return best->second;
}

uint32_t Minisketch32Implementation()
{
    // Fast compute-once idiom.
    static uint32_t best = FindBestImplementation();
    return best;
}

} // namespace


Minisketch MakeMinisketch32(size_t capacity)
{
    return Minisketch(BITS, Minisketch32Implementation(), capacity);
}

Minisketch MakeMinisketch32FP(size_t max_elements, uint32_t fpbits)
{
    return Minisketch::CreateFP(BITS, Minisketch32Implementation(), max_elements, fpbits);
}
} // namespace node
