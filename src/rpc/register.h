// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_REGISTER_H
#define BITCOIN_RPC_REGISTER_H

/** These are in one header file to avoid creating tons of single-function
 * headers for everything under src/rpc/ */
class CRPCTable;

void RegisterBlockchainRPCCommands(CRPCTable &tableRPC);
void RegisterMempoolRPCCommands(CRPCTable&);
void RegisterTxoutProofRPCCommands(CRPCTable&);
void RegisterNetRPCCommands(CRPCTable &tableRPC);
void RegisterMiscRPCCommands(CRPCTable &tableRPC);
void RegisterMiningRPCCommands(CRPCTable &tableRPC);
void RegisterRawTransactionRPCCommands(CRPCTable &tableRPC);
void RegisterSignerRPCCommands(CRPCTable &tableRPC);

static inline void RegisterAllCoreRPCCommands(CRPCTable &t)
{
    RegisterBlockchainRPCCommands(t);
    RegisterMempoolRPCCommands(t);
    RegisterTxoutProofRPCCommands(t);
    RegisterNetRPCCommands(t);
    RegisterMiscRPCCommands(t);
    RegisterMiningRPCCommands(t);
    RegisterRawTransactionRPCCommands(t);
#ifdef ENABLE_EXTERNAL_SIGNER
    RegisterSignerRPCCommands(t);
#endif // ENABLE_EXTERNAL_SIGNER
}

#endif // BITCOIN_RPC_REGISTER_H
