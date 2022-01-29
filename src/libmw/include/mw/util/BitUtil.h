#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <cstdint>

class BitUtil
{
public:
    static uint64_t FillOnesToRight(const uint64_t input) noexcept
    {
        uint64_t x = input;
        x = x | (x >> 1);
        x = x | (x >> 2);
        x = x | (x >> 4);
        x = x | (x >> 8);
        x = x | (x >> 16);
        x = x | (x >> 32);
        return x;
    }

    //
    // Counts the number of bits set to 1.
    //
    static uint8_t CountBitsSet(const uint64_t input) noexcept
    {
        // MW: TODO - use __popcnt64
        uint64_t n = input;
        uint8_t count = 0;
        while (n)
        {
            count += (uint8_t)(n & 1);
            n >>= 1;
        }

        return count;
    }

    static uint8_t CountRightmostZeros(const uint64_t input) noexcept
    {
        assert(input != 0);

        uint8_t count = 0;

        uint64_t n = input;
        while ((n & 1) == 0) {
            ++count;
            n >>= 1;
        }

        return count;
    }
};