// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_SERVICES_RPC_ASSETRPC_H
#define SYSCOIN_SERVICES_RPC_ASSETRPC_H
#include <string>
class CUniValue;
class COutPoint;
unsigned int addressunspent(const std::string& strAddressFrom, COutPoint& outpoint);
UniValue ValueFromAssetAmount(const CAmount& amount, int precision);
CAmount AssetAmountFromValue(UniValue& value, int precision);
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
bool AssetRange(const CAmount& amountIn, int precision);
#endif // SYSCOIN_SERVICES_RPC_ASSETRPC_H
