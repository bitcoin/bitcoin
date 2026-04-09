// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Elektron Net modifications:
//   Currency units:  1 Elek = 100,000,000 Lep (Lepton)
//                    1 Lep  = 100 cLep (Lepton Cent)
//   Maximum supply:  21,000,000 Elek  (identical cap to Bitcoin)

#ifndef BITCOIN_CONSENSUS_AMOUNT_H
#define BITCOIN_CONSENSUS_AMOUNT_H

#include <cstdint>

/** Amount in Lep / Lepton (smallest unit; analogous to satoshi). Can be negative. */
typedef int64_t CAmount;

/** The number of Lep (Lepton) in one Elek. */
static constexpr CAmount COIN = 100000000; // 1 Elek = 10^8 Lep

/** No amount larger than this (in Lep) is valid.
 *
 * Maximum supply of Elektron Net: 21,000,000 Elek – identical to Bitcoin's cap.
 * Enforced by every node; consensus-critical.
 * */
static constexpr CAmount MAX_MONEY = 21000000 * COIN;
inline bool MoneyRange(const CAmount& nValue) { return (nValue >= 0 && nValue <= MAX_MONEY); }

#endif // BITCOIN_CONSENSUS_AMOUNT_H
