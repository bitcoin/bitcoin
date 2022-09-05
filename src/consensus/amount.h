// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_CONSENSUS_AMOUNT_H
#define SYSCOIN_CONSENSUS_AMOUNT_H
// SYSCOIN
#include <unordered_map>
#include <unordered_set>
#include <cstdint>

/** Amount in satoshis (Can be negative) */
typedef int64_t CAmount;

/** The amount of satoshis in one SYS. */
static constexpr CAmount COIN = 100000000;

/** No amount larger than this (in satoshi) is valid.
 *
 * Note that this constant is *not* the total money supply, which in Syscoin
 * currently happens to be less than 888,000,000 SYS for various reasons, but
 * rather a sanity check. As this sanity check is used by consensus-critical
 * validation code, the exact value of the MAX_MONEY constant is consensus
 * critical; in unusual circumstances like a(nother) overflow bug that allowed
 * for the creation of coins out of thin air modification could lead to a fork.
 * */
static const CAmount MAX_MONEY = 1000000000000000000LL - 1LL;
// SYSCOIN
static const CAmount MAX_ASSET = 1000000000000000000LL - 1LL; // 10^18 - 1 max decimal value that will fit in CAmount
static const CAmount COST_ASSET = COIN;
inline bool MoneyRange(const CAmount& nValue) { return (nValue >= 0 && nValue <= MAX_MONEY); }
inline bool MoneyRangeAsset(const CAmount& nValue) { return (nValue >= 0 && nValue <= MAX_ASSET); }
struct AssetMapOutput {
    bool bZeroVal;
    // satoshi amount of all outputs
    CAmount nAmount;
    AssetMapOutput(const bool &bZeroValIn, const CAmount &nAmountIn): bZeroVal(bZeroValIn), nAmount(nAmountIn) {}
    // this is consensus critical, it will ensure input assets and output assets are equal
    friend bool operator==(const AssetMapOutput& a, const AssetMapOutput& b)
    {
        return (a.bZeroVal == b.bZeroVal &&
                a.nAmount  == b.nAmount);
    }

    friend bool operator!=(const AssetMapOutput& a, const AssetMapOutput& b)
    {
        return !(a == b);
    }
};
typedef std::unordered_map<uint64_t, AssetMapOutput> CAssetsMap;
typedef std::unordered_set<uint64_t> CAssetsSet;
#endif // SYSCOIN_CONSENSUS_AMOUNT_H
