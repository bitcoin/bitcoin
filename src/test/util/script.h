// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_SCRIPT_H
#define BITCOIN_TEST_UTIL_SCRIPT_H

/** Flags that are not forbidden by an assert in script validation */
bool IsValidFlagCombination(unsigned flags);

#endif // BITCOIN_TEST_UTIL_SCRIPT_H
