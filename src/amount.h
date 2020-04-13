// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_AMOUNT_H
#define SYSCOIN_AMOUNT_H

#include <stdint.h>

/** Amount in satoshis (Can be negative) */
typedef int64_t CAmount;

static const CAmount COIN = 100000000;

/** No amount larger than this (in satoshi) is valid.
 *
 * Note that this constant is *not* the total money supply, which in Syscoin
 * currently happens to be less than 888,000,000 SYS for various reasons, but
 * rather a sanity check. As this sanity check is used by consensus-critical
 * validation code, the exact value of the MAX_MONEY constant is consensus
 * critical; in unusual circumstances like a(nother) overflow bug that allowed
 * for the creation of coins out of thin air modification could lead to a fork.
 * */
static const CAmount MAX_MONEY = 888000000 * COIN;
inline bool MoneyRange(const CAmount& nValue) { return (nValue >= 0 && nValue <= MAX_MONEY); }

// SYSCOIN
/** Upper bound for mantissa.
* 10^18-1 is the largest arbitrary decimal that will fit in a signed 64-bit integer.
* Larger integers cannot consist of arbitrary combinations of 0-9:
*
*   999999999999999999  10^18-1
*  1000000000000000000  10^18       (would overflow)
*  9223372036854775807  (1<<63)-1   (max int64_t)
*  9999999999999999999  10^19-1     (would overflow)
*/
static const CAmount MAX_ASSET = 1000000000000000000LL - 1LL;
inline bool AssetRange(const CAmount& nValue) { return (nValue > 0 && nValue <= MAX_ASSET); }

#endif //  SYSCOIN_AMOUNT_H
