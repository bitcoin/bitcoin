// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTILTIME_H
#define BITCOIN_UTILTIME_H

#include <stdint.h>
#include <string>

int64_t GetTime();
int64_t GetTimeMillis();
int64_t GetTimeMicros();
int64_t GetLogTimeMicros();
void SetMockTime(int64_t nMockTimeIn);
void MilliSleep(int64_t n);

std::string DateTimeStrFormat(const char* pszFormat, int64_t nTime);
std::string DurationToDHMS(int64_t nDurationTime);

#endif // BITCOIN_UTILTIME_H
