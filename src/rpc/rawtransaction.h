// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_RAWTRANSACTION_H
#define BITCOIN_RPC_RAWTRANSACTION_H

#include <core_io.h>

class Chainstate;
class CTransaction;
class CTxUndo;
class UniValue;
class uint256;

/** Render a decoded transaction with block context from the active chainstate. */
void TxToJSON(const CTransaction& tx, uint256 hashBlock, UniValue& entry,
              Chainstate& active_chainstate, const CTxUndo* txundo = nullptr,
              TxVerbosity verbosity = TxVerbosity::SHOW_DETAILS);

#endif // BITCOIN_RPC_RAWTRANSACTION_H
