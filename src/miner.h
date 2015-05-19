// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MINER_H
#define BITCOIN_MINER_H

#include <stdint.h>

#include "keystore.h"

class Credits_CBlock;
class Credits_CBlockIndex;
struct Credits_CBlockTemplate;
class Credits_CReserveKey;
class CScript;
class Credits_CWallet;
class CPubKey;

/** Run the miner threads */
void GenerateBitcredits(bool fGenerate, Credits_CWallet* pwallet, Credits_CWallet* pdepositWallet, int nThreads, bool coinbaseDepositDisabled);
/** Generate a new block, without valid proof-of-work */
Credits_CBlockTemplate* CreateNewBlock(const CScript& scriptPubKeyCoinbase, const CScript& scriptPubKeyDeposit, const CScript& scriptPubKeyDepositChange, CPubKey& pubKeySigningDeposit, CKeyStore * keystore, Credits_CWallet* pdepositWallet, bool coinbaseDepositDisabled);
Credits_CBlockTemplate* CreateNewBlockWithKey(Credits_CReserveKey& coinbaseReservekey, Credits_CReserveKey& depositReserveKey, Credits_CReserveKey& depositChangeReserveKey, Credits_CReserveKey& depositReserveSigningKey, CKeyStore * bitcreditKeystore, Credits_CWallet* pdepositWallet, bool coinbaseDepositDisabled);
/** Modify the extranonce in a block */
void IncrementExtraNonce(Credits_CBlock* pblock, Credits_CBlockIndex* pindexPrev, unsigned int& nExtraNonce, CKeyStore * bitcreditKeystore, CKeyStore * depositKeystore, const bool& coinbaseDepositDisabled);
/** Do mining precalculation */
void FormatHashBuffers(Credits_CBlock* pblock, char* pmidstate, char* pdata, char* phash1);
/** Check mined block */
bool CheckWork(Credits_CBlock* pblock, Credits_CWallet& wallet, Credits_CWallet& depositWallet, Credits_CReserveKey& reservekey, Credits_CReserveKey& depositReserveKey, Credits_CReserveKey& depositChangeReserveKey, Credits_CReserveKey& depositReserveSigningKey);
/** Base sha256 mining transform */
void SHA256Transform(void* pstate, void* pinput, const void* pinit);

extern double dHashesPerSec;
extern int64_t nHPSTimerStart;

#endif // BITCOIN_MINER_H
