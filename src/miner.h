// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MINER_H
#define BITCOIN_MINER_H

#include <stdint.h>

#include "keystore.h"

class Bitcredit_CBlock;
class Bitcredit_CBlockIndex;
struct Bitcredit_CBlockTemplate;
class Bitcredit_CReserveKey;
class CScript;
class Bitcredit_CWallet;
class CPubKey;

/** Run the miner threads */
void GenerateBitcredits(bool fGenerate, Bitcredit_CWallet* pwallet, Bitcredit_CWallet* pdepositWallet, int nThreads, bool coinbaseDepositDisabled);
/** Generate a new block, without valid proof-of-work */
Bitcredit_CBlockTemplate* CreateNewBlock(const CScript& scriptPubKeyCoinbase, const CScript& scriptPubKeyDeposit, const CScript& scriptPubKeyDepositChange, CPubKey& pubKeySigningDeposit, CKeyStore * keystore, Bitcredit_CWallet* pdepositWallet, bool coinbaseDepositDisabled);
Bitcredit_CBlockTemplate* CreateNewBlockWithKey(Bitcredit_CReserveKey& coinbaseReservekey, Bitcredit_CReserveKey& depositReserveKey, Bitcredit_CReserveKey& depositChangeReserveKey, Bitcredit_CReserveKey& depositReserveSigningKey, CKeyStore * bitcreditKeystore, Bitcredit_CWallet* pdepositWallet, bool coinbaseDepositDisabled);
/** Modify the extranonce in a block */
void IncrementExtraNonce(Bitcredit_CBlock* pblock, Bitcredit_CBlockIndex* pindexPrev, unsigned int& nExtraNonce, CKeyStore * bitcreditKeystore, CKeyStore * depositKeystore, const bool& coinbaseDepositDisabled);
/** Do mining precalculation */
void FormatHashBuffers(Bitcredit_CBlock* pblock, char* pmidstate, char* pdata, char* phash1);
/** Check mined block */
bool CheckWork(Bitcredit_CBlock* pblock, Bitcredit_CWallet& wallet, Bitcredit_CWallet& depositWallet, Bitcredit_CReserveKey& reservekey, Bitcredit_CReserveKey& depositReserveKey, Bitcredit_CReserveKey& depositChangeReserveKey, Bitcredit_CReserveKey& depositReserveSigningKey);
/** Base sha256 mining transform */
void SHA256Transform(void* pstate, void* pinput, const void* pinit);

extern double dHashesPerSec;
extern int64_t nHPSTimerStart;

#endif // BITCOIN_MINER_H
