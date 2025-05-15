// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/fuzz.h>
#include <util/moneystr.h>
#include <util/strencodings.h>

#include <string>

FUZZ_TARGET(parse_numbers)
{
    const std::string random_string(buffer.begin(), buffer.end());
    {
        const auto i8{ToIntegral<int8_t>(random_string)};
        const auto u8{ToIntegral<uint8_t>(random_string)};
        const auto i16{ToIntegral<int16_t>(random_string)};
        const auto u16{ToIntegral<uint16_t>(random_string)};
        const auto i32{ToIntegral<int32_t>(random_string)};
        const auto u32{ToIntegral<uint32_t>(random_string)};
        const auto i64{ToIntegral<int64_t>(random_string)};
        const auto u64{ToIntegral<uint64_t>(random_string)};
        if (i8) {
            assert(i8 == i16);
            if (*i8 > 0) {
                assert(i8 == u8);
            }
        }
        if (i16) {
            assert(i16 == i32);
            if (*i16 > 0) {
                assert(i16 == u16);
            }
        } else {
            assert(!i8);
        }
        if (i32) {
            assert(i32 == i64);
            if (*i32 > 0) {
                assert(i32 == u32);
            }
        } else {
            assert(!i16);
        }
        if (i64) {
            if (*i64 > 0) {
                assert(i64 == u64);
            }
        } else {
            assert(!i32);
        }
    }

    (void)ParseMoney(random_string);

    (void)LocaleIndependentAtoi<int>(random_string);

    int64_t i64;
    (void)LocaleIndependentAtoi<int64_t>(random_string);
    (void)ParseFixedPoint(random_string, 3, &i64);
}
