// Copyright (c) 2016-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_RBF_H
#define BITCOIN_UTIL_RBF_H

#include <cstdint>

class CTransaction;

static const uint32_t MAX_BIP125_RBF_SEQUENCE = 0xfffffffd;

/** Check whether the sequence numbers on this transaction are signaling opt-in to replace-by-fee,
 * according to BIP 125.  Allow opt-out of transaction replacement by setting nSequence >
 * MAX_BIP125_RBF_SEQUENCE (SEQUENCE_FINAL-2) on all inputs.
*
* SEQUENCE_FINAL-1 is picked to still allow use of nLockTime by non-replaceable transactions. All
* inputs rather than just one is for the sake of multi-party protocols, where we don't want a single
* party to be able to disable replacement by opting out in their own input. */
bool SignalsOptInRBF(const CTransaction& tx);

#endif // BITCOIN_UTIL_RBF_H
