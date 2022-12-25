// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_RPC_REGISTER_H
#define SYSCOIN_RPC_REGISTER_H

/** These are in one header file to avoid creating tons of single-function
 * headers for everything under src/rpc/ */
class CRPCTable;

void RegisterBlockchainRPCCommands(CRPCTable &tableRPC);
void RegisterFeeRPCCommands(CRPCTable&);
void RegisterMempoolRPCCommands(CRPCTable&);
void RegisterMiningRPCCommands(CRPCTable &tableRPC);
void RegisterNodeRPCCommands(CRPCTable&);
void RegisterNetRPCCommands(CRPCTable&);
void RegisterOutputScriptRPCCommands(CRPCTable&);
void RegisterRawTransactionRPCCommands(CRPCTable &tableRPC);
// SYSCOIN
/** Register Syscoin Asset RPC commands */
void RegisterAssetRPCCommands(CRPCTable &tableRPC);
/** Register masternode RPC commands */
void RegisterMasternodeRPCCommands(CRPCTable &tableRPC);
/** Register governance RPC commands */
void RegisterGovernanceRPCCommands(CRPCTable &tableRPC);
/** Register Evo RPC commands */
void RegisterEvoRPCCommands(CRPCTable &tableRPC);
/** Register Quorums RPC commands */
void RegisterQuorumsRPCCommands(CRPCTable &tableRPC);
/** Register raw transaction RPC commands */
void RegisterSignMessageRPCCommands(CRPCTable&);
void RegisterSignerRPCCommands(CRPCTable &tableRPC);
void RegisterTxoutProofRPCCommands(CRPCTable&);

static inline void RegisterAllCoreRPCCommands(CRPCTable &t)
{
    RegisterBlockchainRPCCommands(t);
    RegisterFeeRPCCommands(t);
    RegisterMempoolRPCCommands(t);
    RegisterMiningRPCCommands(t);
    RegisterNodeRPCCommands(t);
    RegisterNetRPCCommands(t);
    RegisterOutputScriptRPCCommands(t);
    RegisterRawTransactionRPCCommands(t);
    // SYSCOIN
    RegisterAssetRPCCommands(t);
    RegisterMasternodeRPCCommands(t);
    RegisterGovernanceRPCCommands(t);
    RegisterEvoRPCCommands(t);
    RegisterQuorumsRPCCommands(t);
    RegisterSignMessageRPCCommands(t);
#ifdef ENABLE_EXTERNAL_SIGNER
    RegisterSignerRPCCommands(t);
#endif // ENABLE_EXTERNAL_SIGNER
    RegisterTxoutProofRPCCommands(t);
}

#endif // SYSCOIN_RPC_REGISTER_H
