// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POW_H
#define BITCOIN_POW_H

#include "consensus/params.h"

#include <stdint.h>

class CBlockHeader;
class CBlockIndex;
class arith_uint256;

unsigned int GetNextWorkRequiredLog(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params&);
arith_uint256 GetBlockProof(const CBlockIndex& block);

#endif // BITCOIN_POW_H
