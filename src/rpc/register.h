// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPCREGISTER_H
#define BITCOIN_RPCREGISTER_H

/** These are in one header file to avoid creating tons of single-function
 * headers for everything under src/rpc/ */
class CRPCTable;

/** Register block chain RPC commands */
void RegisterBlockchainRPCCommands(CRPCTable &tableRPC);
/** Register P2P networking RPC commands */
void RegisterNetRPCCommands(CRPCTable &tableRPC);
/** Register miscellaneous RPC commands */
void RegisterMiscRPCCommands(CRPCTable &tableRPC);
/** Register mining RPC commands */
void RegisterMiningRPCCommands(CRPCTable &tableRPC);
/** Register raw transaction RPC commands */
void RegisterRawTransactionRPCCommands(CRPCTable &tableRPC);

/** Register Omni data retrieval RPC commands */
void RegisterOmniDataRetrievalRPCCommands(CRPCTable &tableRPC);
/** Register Omni transaction creation RPC commands */
void RegisterOmniTransactionCreationRPCCommands(CRPCTable &tableRPC);
/** Register Omni payload creation RPC commands */
void RegisterOmniPayloadCreationRPCCommands(CRPCTable &tableRPC);
/** Register Omni raw transaction RPC commands */
void RegisterOmniRawTransactionRPCCommands(CRPCTable &tableRPC);

static inline void RegisterAllCoreRPCCommands(CRPCTable &tableRPC)
{
    RegisterBlockchainRPCCommands(tableRPC);
    RegisterNetRPCCommands(tableRPC);
    RegisterMiscRPCCommands(tableRPC);
    RegisterMiningRPCCommands(tableRPC);
    RegisterRawTransactionRPCCommands(tableRPC);

    /* Omni Core RPCs: */
    RegisterOmniDataRetrievalRPCCommands(tableRPC);
    RegisterOmniTransactionCreationRPCCommands(tableRPC);
    RegisterOmniPayloadCreationRPCCommands(tableRPC);
    RegisterOmniRawTransactionRPCCommands(tableRPC);
}

#endif
