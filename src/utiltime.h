// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTILTIME_H
#define BITCOIN_UTILTIME_H

#include <stdint.h>
#include <string>

/**
 * GetTimeMicros() and GetTimeMillis() both return the system time, but in
 * different units. GetTime() returns the system time in seconds, but also
 * supports mocktime, where the time can be specified by the user, eg for
 * testing (eg with the setmocktime rpc, or -mocktime argument).
 *
 * TODO: Rework these functions to be type-safe (so that we don't inadvertently
 * compare numbers with different units, or compare a mocktime to system time).
 */

int64_t GetTime();
int64_t GetTimeMillis();
int64_t GetTimeMicros();
int64_t GetSystemTimeInSeconds(); // Like GetTime(), but not mockable
void SetMockTime(int64_t nMockTimeIn);
int64_t GetMockTime();
void MilliSleep(int64_t n);

std::string DateTimeStrFormat(const char* pszFormat, int64_t nTime);

#endif // BITCOIN_UTILTIME_H
