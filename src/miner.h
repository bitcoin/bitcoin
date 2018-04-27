// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2013 The NovaCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef NOVACOIN_MINER_H
#define NOVACOIN_MINER_H

#include "main.h"
#include "wallet.h"

/* Generate a new block, without valid proof-of-work */
CBlock* CreateNewBlock(CWallet* pwallet, bool fProofOfStake=false, int64_t* pFees = 0);

/** Modify the extranonce in a block */
void IncrementExtraNonce(CBlock* pblock, CBlockIndex* pindexPrev, unsigned int& nExtraNonce);

/** Do mining precalculation */
void FormatHashBuffers(CBlock* pblock, char* pmidstate, char* pdata, char* phash1);

/** Check mined proof-of-work block */
bool CheckWork(CBlock* pblock, CWallet& wallet, CReserveKey& reservekey);

/** Check mined proof-of-stake block */
bool CheckStake(CBlock* pblock, CWallet& wallet);

/** Base sha256 mining transform */
void SHA256Transform(void* pstate, void* pinput, const void* pinit);

#endif // NOVACOIN_MINER_H
