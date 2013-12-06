// Copyright (c) 2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BITCOINTIME_H
#define BITCOIN_BITCOINTIME_H

#include <stdint.h>
#include <string>

namespace BitcoinTime
{
    int64_t GetTimeMillis();
    int64_t GetTimeMicros();
    std::string DateTimeStrFormat(const char* pszFormat, int64_t nTime);

    int64_t GetTime();
    void SetMockTime(int64_t nMockTimeIn);
    int64_t GetAdjustedTime();
    int64_t GetTimeOffset();
    void SetTimeOffset(int64_t n);
}

#endif
