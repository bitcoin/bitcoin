// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/fuzz.h>
#include <util/moneystr.h>
#include <util/strencodings.h>

#include <string>

FUZZ_TARGET(parse_numbers)
{
    const std::string random_string(buffer.begin(), buffer.end());

    (void)ParseMoney(random_string);

    uint8_t u8;
    (void)ParseUInt8(random_string, &u8);

    uint16_t u16;
    (void)ParseUInt16(random_string, &u16);

    int32_t i32;
    (void)ParseInt32(random_string, &i32);
    (void)LocaleIndependentAtoi<int>(random_string);

    uint32_t u32;
    (void)ParseUInt32(random_string, &u32);

    int64_t i64;
    (void)LocaleIndependentAtoi<int64_t>(random_string);
    (void)ParseFixedPoint(random_string, 3, &i64);
    (void)ParseInt64(random_string, &i64);

    uint64_t u64;
    (void)ParseUInt64(random_string, &u64);
}
