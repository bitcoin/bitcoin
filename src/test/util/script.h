// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_SCRIPT_H
#define BITCOIN_TEST_UTIL_SCRIPT_H

#include <crypto/sha256.h>
#include <script/script.h>
#include <script/standard.h>

static const std::vector<uint8_t> STACK_ELEM_OP_TRUE{uint8_t{OP_TRUE}};
static const CScript P2SH_OP_TRUE{
    CScript{}
    << OP_HASH160
    << ToByteVector(CScriptID{CScript{} << OP_TRUE})
    << OP_EQUAL};

/** Flags that are not forbidden by an assert in script validation */
bool IsValidFlagCombination(unsigned flags);

#endif // BITCOIN_TEST_UTIL_SCRIPT_H
