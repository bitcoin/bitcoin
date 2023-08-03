// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/libstandardness.h>

#include <chainparams.h>
#include <common/args.h>
#include <common/shared_lib_utils.h>
#include <node/blockstorage.h>
#include <node/kernel_notifications.h>
#include <policy/fees.h>
#include <policy/fees_args.h>
#include <timedata.h>
#include <validation.h>

namespace {

inline int set_result(libstandardness_result* ret, libstandardness_result sret)
{
    if (ret)
        *ret = sret;
    return 0;
}

} // namespace

int libstandard_verify_transaction(const unsigned char *txTo, unsigned int txToLen, libstandardness_result* result)
{
    try {
        TxInputStream stream(PROTOCOL_VERSION, txTo, txToLen);
        CTransaction tx(deserialize, stream);

        const auto args = ArgsManager{};
        //TODO: pass the chain type as a library argument
        const auto chainParams = CreateChainParams(args, ChainType::MAIN);

        // We initialize block manager options for the dummy chainstate.
        node::BlockManager::Options blockman_opts {
            .chainparams = *chainParams,
            .blocks_dir = args.GetBlocksDirPath(),
        };

        // We initialize mempool options and mempool for the dummy chainstate.
        const auto fee_estimator = std::make_unique<CBlockPolicyEstimator>(FeeestPath(args));
        //TODO: give an adequate value for internal mempool consistency check
        CTxMemPool::Options mempool_opts {
            .estimator = fee_estimator.get(),
            .check_ratio = 0,
        };

        const auto dummy_mempool = std::make_unique<CTxMemPool>(mempool_opts);

        node::KernelNotifications notifications{};
        ChainstateManager::Options chainman_opts{
            .chainparams = *chainParams,
            .datadir = args.GetDataDirNet(), 
            .adjusted_time_callback = GetAdjustedTime,
            .notifications = notifications,
        };
        const auto chainman = std::make_unique<ChainstateManager>(chainman_opts, blockman_opts);

        const auto dummy_chainstate = std::make_unique<Chainstate>(dummy_mempool.get(), chainman->m_blockman, *chainman);
        //TODO: initialize dummy_chainstate with chain tip as seen by the caller and spent_UTXO

        const auto tx_ref = MakeTransactionRef(tx);
        LOCK(::cs_main);
        // The accept_time is only used for CTxMemPoolEntry construction in PreChecks.
        const auto mempool_result = AcceptToMemoryPool(*dummy_chainstate, tx_ref, /*dummy_timestamp*/ 0, /*bypass_limits=*/false, /*test_accept=*/false);
        if (mempool_result.m_result_type == MempoolAcceptResult::ResultType::VALID) {
            return set_result(result, libstandardness_RESULT_OK);
        }
        return set_result(result, libstandardness_RESULT_INVALID);
    } catch (const std::exception&) {
        return set_result(result, libstandardness_RESULT_INVALID);
    }
}

unsigned int libstandardness_version()
{
    // Just use the API version for now
    return LIBSTANDARD_API_VER;
}
