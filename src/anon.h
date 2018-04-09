// Copyright (c) 2017-2018 The Particl developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#ifndef PARTICL_ANON_H
#define PARTICL_ANON_H

#include <inttypes.h>

#include <primitives/transaction.h>
#include <consensus/validation.h>

#include <txmempool.h>

const size_t MIN_RINGSIZE = 3;
const size_t MAX_RINGSIZE = 32;

const size_t MAX_ANON_INPUTS = 32; // To raise see MLSAG_MAX_ROWS also

const size_t ANON_FEE_MULTIPLIER = 2;


bool VerifyMLSAG(const CTransaction &tx, CValidationState &state);

bool AddKeyImagesToMempool(const CTransaction &tx, CTxMemPool &pool);
bool RemoveKeyImagesFromMempool(const uint256 &hash, const CTxIn &txin, CTxMemPool &pool);

bool AllAnonOutputsUnknown(const CTransaction &tx, CValidationState &state);

bool RollBackRCTIndex(int64_t nLastValidRCTOutput, std::set<CCmpPubKey> &setKi);

bool RewindToCheckpoint(int nCheckPointHeight, int &nBlocks, std::string &sError);

#endif  // PARTICL_ANON_H
