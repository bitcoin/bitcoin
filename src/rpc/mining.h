// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef IOP_RPC_MINING_H
#define IOP_RPC_MINING_H

#include "script/script.h"

#include <univalue.h>

/** Generate blocks (mine) */
UniValue generateBlocks(std::shared_ptr<CReserveScript> coinbaseScript, int nGenerate, uint64_t nMaxTries, bool keepScript,  const std::string &whiteListPrivKey, uint8_t threadId);

/** Check bounds on a command line confirm target */
unsigned int ParseConfirmTarget(const UniValue& value);

#endif
