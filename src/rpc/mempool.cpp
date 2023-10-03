// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/blockchain.h>

#include <kernel/mempool_persist.h>

#include <chainparams.h>
#include <core_io.h>
#include <kernel/mempool_entry.h>
#include <node/mempool_persist_args.h>
#include <node/types.h>
#include <policy/rbf.h>
#include <policy/settings.h>
#include <primitives/transaction.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <txmempool.h>
#include <univalue.h>
#include <util/fs.h>
#include <util/moneystr.h>
#include <util/strencodings.h>
#include <util/time.h>

#include <utility>

using kernel::DumpMempool;

using node::DEFAULT_MAX_BURN_AMOUNT;
using node::DEFAULT_MAX_RAW_TX_FEE_RATE;
using node::MempoolPath;
using node::NodeContext;
using node::TransactionError;
using util::ToString;

static RPCHelpMan sendrawtransaction()
{
    return RPCHelpMan{"sendrawtransaction",
        "\nSubmit a raw transaction (serialized, hex-encoded) to local node and network.\n"
        "\nThe transaction will be sent unconditionally to all peers, so using sendrawtransaction\n"
        "for manual rebroadcast may degrade privacy by leaking the transaction's origin, as\n"
        "nodes will normally not rebroadcast non-wallet transactions already in their mempool.\n"
        "\nA specific exception, RPC_TRANSACTION_ALREADY_IN_CHAIN, may throw if the transaction cannot be added to the mempool.\n"
        "\nRelated RPCs: createrawtransaction, signrawtransactionwithkey\n",
        {
            {"hexstring", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hex string of the raw transaction"},
            {"maxfeerate", RPCArg::Type::AMOUNT, RPCArg::Default{FormatMoney(DEFAULT_MAX_RAW_TX_FEE_RATE.GetFeePerK())},
             "Reject transactions whose fee rate is higher than the specified value, expressed in " + CURRENCY_UNIT +
                 "/kvB.\nFee rates larger than 1BTC/kvB are rejected.\nSet to 0 to accept any fee rate."},
            {"maxburnamount", RPCArg::Type::AMOUNT, RPCArg::Default{FormatMoney(DEFAULT_MAX_BURN_AMOUNT)},
             "Reject transactions with provably unspendable outputs (e.g. 'datacarrier' outputs that use the OP_RETURN opcode) greater than the specified value, expressed in " + CURRENCY_UNIT + ".\n"
             "If burning funds through unspendable outputs is desired, increase this value.\n"
             "This check is based on heuristics and does not guarantee spendability of outputs.\n"},
        },
        RPCResult{
            RPCResult::Type::STR_HEX, "", "The transaction hash in hex"
        },
        RPCExamples{
            "\nCreate a transaction\n"
            + HelpExampleCli("createrawtransaction", "\"[{\\\"txid\\\" : \\\"mytxid\\\",\\\"vout\\\":0}]\" \"{\\\"myaddress\\\":0.01}\"") +
            "Sign the transaction, and get back the hex\n"
            + HelpExampleCli("signrawtransactionwithwallet", "\"myhex\"") +
            "\nSend the transaction (signed hex)\n"
            + HelpExampleCli("sendrawtransaction", "\"signedhex\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("sendrawtransaction", "\"signedhex\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            const CAmount max_burn_amount = request.params[2].isNull() ? 0 : AmountFromValue(request.params[2]);

            CMutableTransaction mtx;
            if (!DecodeHexTx(mtx, request.params[0].get_str())) {
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed. Make sure the tx has at least one input.");
            }

            for (const auto& out : mtx.vout) {
                if((out.scriptPubKey.IsUnspendable() || !out.scriptPubKey.HasValidOps()) && out.nValue > max_burn_amount) {
                    throw JSONRPCTransactionError(TransactionError::MAX_BURN_EXCEEDED);
                }
            }

            CTransactionRef tx(MakeTransactionRef(std::move(mtx)));

            const CFeeRate max_raw_tx_fee_rate{ParseFeeRate(self.Arg<UniValue>("maxfeerate"))};

            int64_t virtual_size = GetVirtualTransactionSize(*tx);
            CAmount max_raw_tx_fee = max_raw_tx_fee_rate.GetFee(virtual_size);

            std::string err_string;
            AssertLockNotHeld(cs_main);
            NodeContext& node = EnsureAnyNodeContext(request.context);
            const TransactionError err = BroadcastTransaction(node, tx, err_string, max_raw_tx_fee, /*relay=*/true, /*wait_callback=*/true);
            if (TransactionError::OK != err) {
                throw JSONRPCTransactionError(err, err_string);
            }

            return tx->GetHash().GetHex();
        },
    };
}

static RPCHelpMan testmempoolaccept()
{
    return RPCHelpMan{"testmempoolaccept",
        "\nReturns result of mempool acceptance tests indicating if raw transaction(s) (serialized, hex-encoded) would be accepted by mempool.\n"
        "\nIf multiple transactions are passed in, parents must come before children and package policies apply: the transactions cannot conflict with any mempool transactions or each other.\n"
        "\nIf one transaction fails, other transactions may not be fully validated (the 'allowed' key will be blank).\n"
        "\nThe maximum number of transactions allowed is " + ToString(MAX_PACKAGE_COUNT) + ".\n"
        "\nThis checks if transactions violate the consensus or policy rules.\n"
        "\nSee sendrawtransaction call.\n",
        {
            {"rawtxs", RPCArg::Type::ARR, RPCArg::Optional::NO, "An array of hex strings of raw transactions.",
                {
                    {"rawtx", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, ""},
                },
            },
            {"maxfeerate", RPCArg::Type::AMOUNT, RPCArg::Default{FormatMoney(DEFAULT_MAX_RAW_TX_FEE_RATE.GetFeePerK())},
             "Reject transactions whose fee rate is higher than the specified value, expressed in " + CURRENCY_UNIT +
                 "/kvB.\nFee rates larger than 1BTC/kvB are rejected.\nSet to 0 to accept any fee rate."},
        },
        RPCResult{
            RPCResult::Type::ARR, "", "The result of the mempool acceptance test for each raw transaction in the input array.\n"
                                      "Returns results for each transaction in the same order they were passed in.\n"
                                      "Transactions that cannot be fully validated due to failures in other transactions will not contain an 'allowed' result.\n",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR_HEX, "txid", "The transaction hash in hex"},
                    {RPCResult::Type::STR_HEX, "wtxid", "The transaction witness hash in hex"},
                    {RPCResult::Type::STR, "package-error", /*optional=*/true, "Package validation error, if any (only possible if rawtxs had more than 1 transaction)."},
                    {RPCResult::Type::BOOL, "allowed", /*optional=*/true, "Whether this tx would be accepted to the mempool and pass client-specified maxfeerate. "
                                                       "If not present, the tx was not fully validated due to a failure in another tx in the list."},
                    {RPCResult::Type::NUM, "vsize", /*optional=*/true, "Virtual transaction size as defined in BIP 141. This is different from actual serialized size for witness transactions as witness data is discounted (only present when 'allowed' is true)"},
                    {RPCResult::Type::OBJ, "fees", /*optional=*/true, "Transaction fees (only present if 'allowed' is true)",
                    {
                        {RPCResult::Type::STR_AMOUNT, "base", "transaction fee in " + CURRENCY_UNIT},
                        {RPCResult::Type::STR_AMOUNT, "effective-feerate", /*optional=*/false, "the effective feerate in " + CURRENCY_UNIT + " per KvB. May differ from the base feerate if, for example, there are modified fees from prioritisetransaction or a package feerate was used."},
                        {RPCResult::Type::ARR, "effective-includes", /*optional=*/false, "transactions whose fees and vsizes are included in effective-feerate.",
                            {RPCResult{RPCResult::Type::STR_HEX, "", "transaction wtxid in hex"},
                        }},
                    }},
                    {RPCResult::Type::STR, "reject-reason", /*optional=*/true, "Rejection string (only present when 'allowed' is false)"},
                }},
            }
        },
        RPCExamples{
            "\nCreate a transaction\n"
            + HelpExampleCli("createrawtransaction", "\"[{\\\"txid\\\" : \\\"mytxid\\\",\\\"vout\\\":0}]\" \"{\\\"myaddress\\\":0.01}\"") +
            "Sign the transaction, and get back the hex\n"
            + HelpExampleCli("signrawtransactionwithwallet", "\"myhex\"") +
            "\nTest acceptance of the transaction (signed hex)\n"
            + HelpExampleCli("testmempoolaccept", R"('["signedhex"]')") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("testmempoolaccept", "[\"signedhex\"]")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            const UniValue raw_transactions = request.params[0].get_array();
            if (raw_transactions.size() < 1 || raw_transactions.size() > MAX_PACKAGE_COUNT) {
                throw JSONRPCError(RPC_INVALID_PARAMETER,
                                   "Array must contain between 1 and " + ToString(MAX_PACKAGE_COUNT) + " transactions.");
            }

            const CFeeRate max_raw_tx_fee_rate{ParseFeeRate(self.Arg<UniValue>("maxfeerate"))};

            std::vector<CTransactionRef> txns;
            txns.reserve(raw_transactions.size());
            for (const auto& rawtx : raw_transactions.getValues()) {
                CMutableTransaction mtx;
                if (!DecodeHexTx(mtx, rawtx.get_str())) {
                    throw JSONRPCError(RPC_DESERIALIZATION_ERROR,
                                       "TX decode failed: " + rawtx.get_str() + " Make sure the tx has at least one input.");
                }
                txns.emplace_back(MakeTransactionRef(std::move(mtx)));
            }

            NodeContext& node = EnsureAnyNodeContext(request.context);
            CTxMemPool& mempool = EnsureMemPool(node);
            ChainstateManager& chainman = EnsureChainman(node);
            Chainstate& chainstate = chainman.ActiveChainstate();
            const PackageMempoolAcceptResult package_result = [&] {
                LOCK(::cs_main);
                if (txns.size() > 1) return ProcessNewPackage(chainstate, mempool, txns, /*test_accept=*/true, /*client_maxfeerate=*/{});
                return PackageMempoolAcceptResult(txns[0]->GetWitnessHash(),
                                                  chainman.ProcessTransaction(txns[0], /*test_accept=*/true));
            }();

            UniValue rpc_result(UniValue::VARR);
            // We will check transaction fees while we iterate through txns in order. If any transaction fee
            // exceeds maxfeerate, we will leave the rest of the validation results blank, because it
            // doesn't make sense to return a validation result for a transaction if its ancestor(s) would
            // not be submitted.
            bool exit_early{false};
            for (const auto& tx : txns) {
                UniValue result_inner(UniValue::VOBJ);
                result_inner.pushKV("txid", tx->GetHash().GetHex());
                result_inner.pushKV("wtxid", tx->GetWitnessHash().GetHex());
                if (package_result.m_state.GetResult() == PackageValidationResult::PCKG_POLICY) {
                    result_inner.pushKV("package-error", package_result.m_state.ToString());
                }
                auto it = package_result.m_tx_results.find(tx->GetWitnessHash());
                if (exit_early || it == package_result.m_tx_results.end()) {
                    // Validation unfinished. Just return the txid and wtxid.
                    rpc_result.push_back(std::move(result_inner));
                    continue;
                }
                const auto& tx_result = it->second;
                // Package testmempoolaccept doesn't allow transactions to already be in the mempool.
                CHECK_NONFATAL(tx_result.m_result_type != MempoolAcceptResult::ResultType::MEMPOOL_ENTRY);
                if (tx_result.m_result_type == MempoolAcceptResult::ResultType::VALID) {
                    const CAmount fee = tx_result.m_base_fees.value();
                    // Check that fee does not exceed maximum fee
                    const int64_t virtual_size = tx_result.m_vsize.value();
                    const CAmount max_raw_tx_fee = max_raw_tx_fee_rate.GetFee(virtual_size);
                    if (max_raw_tx_fee && fee > max_raw_tx_fee) {
                        result_inner.pushKV("allowed", false);
                        result_inner.pushKV("reject-reason", "max-fee-exceeded");
                        exit_early = true;
                    } else {
                        // Only return the fee and vsize if the transaction would pass ATMP.
                        // These can be used to calculate the feerate.
                        result_inner.pushKV("allowed", true);
                        result_inner.pushKV("vsize", virtual_size);
                        UniValue fees(UniValue::VOBJ);
                        fees.pushKV("base", ValueFromAmount(fee));
                        fees.pushKV("effective-feerate", ValueFromAmount(tx_result.m_effective_feerate.value().GetFeePerK()));
                        UniValue effective_includes_res(UniValue::VARR);
                        for (const auto& wtxid : tx_result.m_wtxids_fee_calculations.value()) {
                            effective_includes_res.push_back(wtxid.ToString());
                        }
                        fees.pushKV("effective-includes", std::move(effective_includes_res));
                        result_inner.pushKV("fees", std::move(fees));
                    }
                } else {
                    result_inner.pushKV("allowed", false);
                    const TxValidationState state = tx_result.m_state;
                    if (state.GetResult() == TxValidationResult::TX_MISSING_INPUTS) {
                        result_inner.pushKV("reject-reason", "missing-inputs");
                    } else {
                        result_inner.pushKV("reject-reason", state.GetRejectReason());
                    }
                }
                rpc_result.push_back(std::move(result_inner));
            }
            return rpc_result;
        },
    };
}

static std::vector<RPCResult> MempoolEntryDescription()
{
    return {
        RPCResult{RPCResult::Type::NUM, "vsize", "virtual transaction size as defined in BIP 141. This is different from actual serialized size for witness transactions as witness data is discounted."},
        RPCResult{RPCResult::Type::NUM, "weight", "transaction weight as defined in BIP 141."},
        RPCResult{RPCResult::Type::NUM_TIME, "time", "local time transaction entered pool in seconds since 1 Jan 1970 GMT"},
        RPCResult{RPCResult::Type::NUM, "height", "block height when transaction entered pool"},
        RPCResult{RPCResult::Type::NUM, "descendantcount", "number of in-mempool descendant transactions (including this one)"},
        RPCResult{RPCResult::Type::NUM, "descendantsize", "virtual transaction size of in-mempool descendants (including this one)"},
        RPCResult{RPCResult::Type::NUM, "ancestorcount", "number of in-mempool ancestor transactions (including this one)"},
        RPCResult{RPCResult::Type::NUM, "ancestorsize", "virtual transaction size of in-mempool ancestors (including this one)"},
        RPCResult{RPCResult::Type::STR_HEX, "wtxid", "hash of serialized transaction, including witness data"},
        RPCResult{RPCResult::Type::OBJ, "fees", "",
            {
                RPCResult{RPCResult::Type::STR_AMOUNT, "base", "transaction fee, denominated in " + CURRENCY_UNIT},
                RPCResult{RPCResult::Type::STR_AMOUNT, "modified", "transaction fee with fee deltas used for mining priority, denominated in " + CURRENCY_UNIT},
                RPCResult{RPCResult::Type::STR_AMOUNT, "ancestor", "transaction fees of in-mempool ancestors (including this one) with fee deltas used for mining priority, denominated in " + CURRENCY_UNIT},
                RPCResult{RPCResult::Type::STR_AMOUNT, "descendant", "transaction fees of in-mempool descendants (including this one) with fee deltas used for mining priority, denominated in " + CURRENCY_UNIT},
            }},
        RPCResult{RPCResult::Type::ARR, "depends", "unconfirmed transactions used as inputs for this transaction",
            {RPCResult{RPCResult::Type::STR_HEX, "transactionid", "parent transaction id"}}},
        RPCResult{RPCResult::Type::ARR, "spentby", "unconfirmed transactions spending outputs from this transaction",
            {RPCResult{RPCResult::Type::STR_HEX, "transactionid", "child transaction id"}}},
        RPCResult{RPCResult::Type::BOOL, "bip125-replaceable", "Whether this transaction signals BIP125 replaceability or has an unconfirmed ancestor signaling BIP125 replaceability.\n"},
        RPCResult{RPCResult::Type::BOOL, "unbroadcast", "Whether this transaction is currently unbroadcast (initial broadcast not yet acknowledged by any peers)"},
    };
}

static void entryToJSON(const CTxMemPool& pool, UniValue& info, const CTxMemPoolEntry& e) EXCLUSIVE_LOCKS_REQUIRED(pool.cs)
{
    AssertLockHeld(pool.cs);

    size_t ancestor_size{0}, descendant_size{0};
    size_t ancestor_count{0}, descendant_count{0};
    CAmount ancestor_fees{0}, descendant_fees{0};
    pool.CalculateAncestorData(e, ancestor_count, ancestor_size, ancestor_fees);
    pool.CalculateDescendantData(e, descendant_count, descendant_size, descendant_fees);

    info.pushKV("vsize", (int)e.GetTxSize());
    info.pushKV("weight", (int)e.GetTxWeight());
    info.pushKV("time", count_seconds(e.GetTime()));
    info.pushKV("height", (int)e.GetHeight());
    info.pushKV("descendantcount", descendant_count);
    info.pushKV("descendantsize", descendant_size);
    info.pushKV("ancestorcount", ancestor_count);
    info.pushKV("ancestorsize", ancestor_size);
    info.pushKV("wtxid", e.GetTx().GetWitnessHash().ToString());

    UniValue fees(UniValue::VOBJ);
    fees.pushKV("base", ValueFromAmount(e.GetFee()));
    fees.pushKV("modified", ValueFromAmount(e.GetModifiedFee()));
    fees.pushKV("ancestor", ValueFromAmount(ancestor_fees));
    fees.pushKV("descendant", ValueFromAmount(descendant_fees));
    info.pushKV("fees", std::move(fees));

    const CTransaction& tx = e.GetTx();
    std::set<std::string> setDepends;
    for (const CTxIn& txin : tx.vin)
    {
        if (pool.exists(GenTxid::Txid(txin.prevout.hash)))
            setDepends.insert(txin.prevout.hash.ToString());
    }

    UniValue depends(UniValue::VARR);
    for (const std::string& dep : setDepends)
    {
        depends.push_back(dep);
    }

    info.pushKV("depends", std::move(depends));

    UniValue spent(UniValue::VARR);
    for (const CTxMemPoolEntry& child : e.GetMemPoolChildrenConst()) {
        spent.push_back(child.GetTx().GetHash().ToString());
    }

    info.pushKV("spentby", std::move(spent));

    // Add opt-in RBF status
    bool rbfStatus = false;
    RBFTransactionState rbfState = IsRBFOptIn(tx, pool);
    if (rbfState == RBFTransactionState::UNKNOWN) {
        throw JSONRPCError(RPC_MISC_ERROR, "Transaction is not in mempool");
    } else if (rbfState == RBFTransactionState::REPLACEABLE_BIP125) {
        rbfStatus = true;
    }

    info.pushKV("bip125-replaceable", rbfStatus);
    info.pushKV("unbroadcast", pool.IsUnbroadcastTx(tx.GetHash()));
}

UniValue MempoolToJSON(const CTxMemPool& pool, bool verbose, bool include_mempool_sequence)
{
    if (verbose) {
        if (include_mempool_sequence) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Verbose results cannot contain mempool sequence values.");
        }
        LOCK(pool.cs);
        UniValue o(UniValue::VOBJ);
        for (const CTxMemPoolEntry& e : pool.entryAll()) {
            UniValue info(UniValue::VOBJ);
            entryToJSON(pool, info, e);
            // Mempool has unique entries so there is no advantage in using
            // UniValue::pushKV, which checks if the key already exists in O(N).
            // UniValue::pushKVEnd is used instead which currently is O(1).
            o.pushKVEnd(e.GetTx().GetHash().ToString(), std::move(info));
        }
        return o;
    } else {
        UniValue a(UniValue::VARR);
        uint64_t mempool_sequence;
        {
            LOCK(pool.cs);
            for (const CTxMemPoolEntry& e : pool.entryAll()) {
                a.push_back(e.GetTx().GetHash().ToString());
            }
            mempool_sequence = pool.GetSequence();
        }
        if (!include_mempool_sequence) {
            return a;
        } else {
            UniValue o(UniValue::VOBJ);
            o.pushKV("txids", std::move(a));
            o.pushKV("mempool_sequence", mempool_sequence);
            return o;
        }
    }
}

static RPCHelpMan getrawmempool()
{
    return RPCHelpMan{"getrawmempool",
        "\nReturns all transaction ids in memory pool as a json array of string transaction ids.\n"
        "\nHint: use getmempoolentry to fetch a specific transaction from the mempool.\n",
        {
            {"verbose", RPCArg::Type::BOOL, RPCArg::Default{false}, "True for a json object, false for array of transaction ids"},
            {"mempool_sequence", RPCArg::Type::BOOL, RPCArg::Default{false}, "If verbose=false, returns a json object with transaction list and mempool sequence number attached."},
        },
        {
            RPCResult{"for verbose = false",
                RPCResult::Type::ARR, "", "",
                {
                    {RPCResult::Type::STR_HEX, "", "The transaction id"},
                }},
            RPCResult{"for verbose = true",
                RPCResult::Type::OBJ_DYN, "", "",
                {
                    {RPCResult::Type::OBJ, "transactionid", "", MempoolEntryDescription()},
                }},
            RPCResult{"for verbose = false and mempool_sequence = true",
                RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::ARR, "txids", "",
                    {
                        {RPCResult::Type::STR_HEX, "", "The transaction id"},
                    }},
                    {RPCResult::Type::NUM, "mempool_sequence", "The mempool sequence value."},
                }},
        },
        RPCExamples{
            HelpExampleCli("getrawmempool", "true")
            + HelpExampleRpc("getrawmempool", "true")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    bool fVerbose = false;
    if (!request.params[0].isNull())
        fVerbose = request.params[0].get_bool();

    bool include_mempool_sequence = false;
    if (!request.params[1].isNull()) {
        include_mempool_sequence = request.params[1].get_bool();
    }

    return MempoolToJSON(EnsureAnyMemPool(request.context), fVerbose, include_mempool_sequence);
},
    };
}

static RPCHelpMan getmempoolancestors()
{
    return RPCHelpMan{"getmempoolancestors",
        "\nIf txid is in the mempool, returns all in-mempool ancestors.\n",
        {
            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id (must be in mempool)"},
            {"verbose", RPCArg::Type::BOOL, RPCArg::Default{false}, "True for a json object, false for array of transaction ids"},
        },
        {
            RPCResult{"for verbose = false",
                RPCResult::Type::ARR, "", "",
                {{RPCResult::Type::STR_HEX, "", "The transaction id of an in-mempool ancestor transaction"}}},
            RPCResult{"for verbose = true",
                RPCResult::Type::OBJ_DYN, "", "",
                {
                    {RPCResult::Type::OBJ, "transactionid", "", MempoolEntryDescription()},
                }},
        },
        RPCExamples{
            HelpExampleCli("getmempoolancestors", "\"mytxid\"")
            + HelpExampleRpc("getmempoolancestors", "\"mytxid\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    bool fVerbose = false;
    if (!request.params[1].isNull())
        fVerbose = request.params[1].get_bool();

    uint256 hash = ParseHashV(request.params[0], "parameter 1");

    const CTxMemPool& mempool = EnsureAnyMemPool(request.context);
    LOCK(mempool.cs);

    const auto entry{mempool.GetEntry(Txid::FromUint256(hash))};
    if (entry == nullptr) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not in mempool");
    }

    auto ancestors{mempool.AssumeCalculateMemPoolAncestors(self.m_name, *entry, CTxMemPool::Limits::NoLimits(), /*fSearchForParents=*/false)};

    if (!fVerbose) {
        UniValue o(UniValue::VARR);
        for (CTxMemPool::txiter ancestorIt : ancestors) {
            o.push_back(ancestorIt->GetTx().GetHash().ToString());
        }
        return o;
    } else {
        UniValue o(UniValue::VOBJ);
        for (CTxMemPool::txiter ancestorIt : ancestors) {
            const CTxMemPoolEntry &e = *ancestorIt;
            const uint256& _hash = e.GetTx().GetHash();
            UniValue info(UniValue::VOBJ);
            entryToJSON(mempool, info, e);
            o.pushKV(_hash.ToString(), std::move(info));
        }
        return o;
    }
},
    };
}

static RPCHelpMan getmempooldescendants()
{
    return RPCHelpMan{"getmempooldescendants",
        "\nIf txid is in the mempool, returns all in-mempool descendants.\n",
        {
            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id (must be in mempool)"},
            {"verbose", RPCArg::Type::BOOL, RPCArg::Default{false}, "True for a json object, false for array of transaction ids"},
        },
        {
            RPCResult{"for verbose = false",
                RPCResult::Type::ARR, "", "",
                {{RPCResult::Type::STR_HEX, "", "The transaction id of an in-mempool descendant transaction"}}},
            RPCResult{"for verbose = true",
                RPCResult::Type::OBJ_DYN, "", "",
                {
                    {RPCResult::Type::OBJ, "transactionid", "", MempoolEntryDescription()},
                }},
        },
        RPCExamples{
            HelpExampleCli("getmempooldescendants", "\"mytxid\"")
            + HelpExampleRpc("getmempooldescendants", "\"mytxid\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    bool fVerbose = false;
    if (!request.params[1].isNull())
        fVerbose = request.params[1].get_bool();

    uint256 hash = ParseHashV(request.params[0], "parameter 1");

    const CTxMemPool& mempool = EnsureAnyMemPool(request.context);
    LOCK(mempool.cs);

    const auto it{mempool.GetIter(hash)};
    if (!it) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not in mempool");
    }

    CTxMemPool::setEntries setDescendants;
    mempool.CalculateDescendants(*it, setDescendants);
    // CTxMemPool::CalculateDescendants will include the given tx
    setDescendants.erase(*it);

    if (!fVerbose) {
        UniValue o(UniValue::VARR);
        for (CTxMemPool::txiter descendantIt : setDescendants) {
            o.push_back(descendantIt->GetTx().GetHash().ToString());
        }

        return o;
    } else {
        UniValue o(UniValue::VOBJ);
        for (CTxMemPool::txiter descendantIt : setDescendants) {
            const CTxMemPoolEntry &e = *descendantIt;
            const uint256& _hash = e.GetTx().GetHash();
            UniValue info(UniValue::VOBJ);
            entryToJSON(mempool, info, e);
            o.pushKV(_hash.ToString(), std::move(info));
        }
        return o;
    }
},
    };
}

static RPCHelpMan getmempoolentry()
{
    return RPCHelpMan{"getmempoolentry",
        "\nReturns mempool data for given transaction\n",
        {
            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id (must be in mempool)"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "", MempoolEntryDescription()},
        RPCExamples{
            HelpExampleCli("getmempoolentry", "\"mytxid\"")
            + HelpExampleRpc("getmempoolentry", "\"mytxid\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    uint256 hash = ParseHashV(request.params[0], "parameter 1");

    const CTxMemPool& mempool = EnsureAnyMemPool(request.context);
    LOCK(mempool.cs);

    const auto entry{mempool.GetEntry(Txid::FromUint256(hash))};
    if (entry == nullptr) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not in mempool");
    }

    UniValue info(UniValue::VOBJ);
    entryToJSON(mempool, info, *entry);
    return info;
},
    };
}

static RPCHelpMan gettxspendingprevout()
{
    return RPCHelpMan{"gettxspendingprevout",
        "Scans the mempool to find transactions spending any of the given outputs",
        {
            {"outputs", RPCArg::Type::ARR, RPCArg::Optional::NO, "The transaction outputs that we want to check, and within each, the txid (string) vout (numeric).",
                {
                    {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                        {
                            {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The transaction id"},
                            {"vout", RPCArg::Type::NUM, RPCArg::Optional::NO, "The output number"},
                        },
                    },
                },
            },
        },
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR_HEX, "txid", "the transaction id of the checked output"},
                    {RPCResult::Type::NUM, "vout", "the vout value of the checked output"},
                    {RPCResult::Type::STR_HEX, "spendingtxid", /*optional=*/true, "the transaction id of the mempool transaction spending this output (omitted if unspent)"},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("gettxspendingprevout", "\"[{\\\"txid\\\":\\\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\\\",\\\"vout\\\":3}]\"")
            + HelpExampleRpc("gettxspendingprevout", "\"[{\\\"txid\\\":\\\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\\\",\\\"vout\\\":3}]\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            const UniValue& output_params = request.params[0].get_array();
            if (output_params.empty()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, outputs are missing");
            }

            std::vector<COutPoint> prevouts;
            prevouts.reserve(output_params.size());

            for (unsigned int idx = 0; idx < output_params.size(); idx++) {
                const UniValue& o = output_params[idx].get_obj();

                RPCTypeCheckObj(o,
                                {
                                    {"txid", UniValueType(UniValue::VSTR)},
                                    {"vout", UniValueType(UniValue::VNUM)},
                                }, /*fAllowNull=*/false, /*fStrict=*/true);

                const Txid txid = Txid::FromUint256(ParseHashO(o, "txid"));
                const int nOutput{o.find_value("vout").getInt<int>()};
                if (nOutput < 0) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, vout cannot be negative");
                }

                prevouts.emplace_back(txid, nOutput);
            }

            const CTxMemPool& mempool = EnsureAnyMemPool(request.context);
            LOCK(mempool.cs);

            UniValue result{UniValue::VARR};

            for (const COutPoint& prevout : prevouts) {
                UniValue o(UniValue::VOBJ);
                o.pushKV("txid", prevout.hash.ToString());
                o.pushKV("vout", (uint64_t)prevout.n);

                const CTransaction* spendingTx = mempool.GetConflictTx(prevout);
                if (spendingTx != nullptr) {
                    o.pushKV("spendingtxid", spendingTx->GetHash().ToString());
                }

                result.push_back(std::move(o));
            }

            return result;
        },
    };
}

UniValue MempoolInfoToJSON(const CTxMemPool& pool)
{
    // Make sure this call is atomic in the pool.
    LOCK(pool.cs);
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("loaded", pool.GetLoadTried());
    ret.pushKV("size", (int64_t)pool.size());
    ret.pushKV("bytes", (int64_t)pool.GetTotalTxSize());
    ret.pushKV("usage", (int64_t)pool.DynamicMemoryUsage());
    ret.pushKV("total_fee", ValueFromAmount(pool.GetTotalFee()));
    ret.pushKV("maxmempool", pool.m_opts.max_size_bytes);
    ret.pushKV("mempoolminfee", ValueFromAmount(std::max(pool.GetMinFee(), pool.m_opts.min_relay_feerate).GetFeePerK()));
    ret.pushKV("minrelaytxfee", ValueFromAmount(pool.m_opts.min_relay_feerate.GetFeePerK()));
    ret.pushKV("incrementalrelayfee", ValueFromAmount(pool.m_opts.incremental_relay_feerate.GetFeePerK()));
    ret.pushKV("unbroadcastcount", uint64_t{pool.GetUnbroadcastTxs().size()});
    ret.pushKV("fullrbf", pool.m_opts.full_rbf);
    return ret;
}

static RPCHelpMan getmempoolinfo()
{
    return RPCHelpMan{"getmempoolinfo",
        "Returns details on the active state of the TX memory pool.",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::BOOL, "loaded", "True if the initial load attempt of the persisted mempool finished"},
                {RPCResult::Type::NUM, "size", "Current tx count"},
                {RPCResult::Type::NUM, "bytes", "Sum of all virtual transaction sizes as defined in BIP 141. Differs from actual serialized size because witness data is discounted"},
                {RPCResult::Type::NUM, "usage", "Total memory usage for the mempool"},
                {RPCResult::Type::STR_AMOUNT, "total_fee", "Total fees for the mempool in " + CURRENCY_UNIT + ", ignoring modified fees through prioritisetransaction"},
                {RPCResult::Type::NUM, "maxmempool", "Maximum memory usage for the mempool"},
                {RPCResult::Type::STR_AMOUNT, "mempoolminfee", "Minimum fee rate in " + CURRENCY_UNIT + "/kvB for tx to be accepted. Is the maximum of minrelaytxfee and minimum mempool fee"},
                {RPCResult::Type::STR_AMOUNT, "minrelaytxfee", "Current minimum relay fee for transactions"},
                {RPCResult::Type::NUM, "incrementalrelayfee", "minimum fee rate increment for mempool limiting or replacement in " + CURRENCY_UNIT + "/kvB"},
                {RPCResult::Type::NUM, "unbroadcastcount", "Current number of transactions that haven't passed initial broadcast yet"},
                {RPCResult::Type::BOOL, "fullrbf", "True if the mempool accepts RBF without replaceability signaling inspection"},
            }},
        RPCExamples{
            HelpExampleCli("getmempoolinfo", "")
            + HelpExampleRpc("getmempoolinfo", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    return MempoolInfoToJSON(EnsureAnyMemPool(request.context));
},
    };
}

static RPCHelpMan importmempool()
{
    return RPCHelpMan{
        "importmempool",
        "Import a mempool.dat file and attempt to add its contents to the mempool.\n"
        "Warning: Importing untrusted files is dangerous, especially if metadata from the file is taken over.",
        {
            {"filepath", RPCArg::Type::STR, RPCArg::Optional::NO, "The mempool file"},
            {"options",
             RPCArg::Type::OBJ_NAMED_PARAMS,
             RPCArg::Optional::OMITTED,
             "",
             {
                 {"use_current_time", RPCArg::Type::BOOL, RPCArg::Default{true},
                  "Whether to use the current system time or use the entry time metadata from the mempool file.\n"
                  "Warning: Importing untrusted metadata may lead to unexpected issues and undesirable behavior."},
                 {"apply_fee_delta_priority", RPCArg::Type::BOOL, RPCArg::Default{false},
                  "Whether to apply the fee delta metadata from the mempool file.\n"
                  "It will be added to any existing fee deltas.\n"
                  "The fee delta can be set by the prioritisetransaction RPC.\n"
                  "Warning: Importing untrusted metadata may lead to unexpected issues and undesirable behavior.\n"
                  "Only set this bool if you understand what it does."},
                 {"apply_unbroadcast_set", RPCArg::Type::BOOL, RPCArg::Default{false},
                  "Whether to apply the unbroadcast set metadata from the mempool file.\n"
                  "Warning: Importing untrusted metadata may lead to unexpected issues and undesirable behavior."},
             },
             RPCArgOptions{.oneline_description = "options"}},
        },
        RPCResult{RPCResult::Type::OBJ, "", "", std::vector<RPCResult>{}},
        RPCExamples{HelpExampleCli("importmempool", "/path/to/mempool.dat") + HelpExampleRpc("importmempool", "/path/to/mempool.dat")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            const NodeContext& node{EnsureAnyNodeContext(request.context)};

            CTxMemPool& mempool{EnsureMemPool(node)};
            ChainstateManager& chainman = EnsureChainman(node);
            Chainstate& chainstate = chainman.ActiveChainstate();

            if (chainman.IsInitialBlockDownload()) {
                throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "Can only import the mempool after the block download and sync is done.");
            }

            const fs::path load_path{fs::u8path(request.params[0].get_str())};
            const UniValue& use_current_time{request.params[1]["use_current_time"]};
            const UniValue& apply_fee_delta{request.params[1]["apply_fee_delta_priority"]};
            const UniValue& apply_unbroadcast{request.params[1]["apply_unbroadcast_set"]};
            kernel::ImportMempoolOptions opts{
                .use_current_time = use_current_time.isNull() ? true : use_current_time.get_bool(),
                .apply_fee_delta_priority = apply_fee_delta.isNull() ? false : apply_fee_delta.get_bool(),
                .apply_unbroadcast_set = apply_unbroadcast.isNull() ? false : apply_unbroadcast.get_bool(),
            };

            if (!kernel::LoadMempool(mempool, load_path, chainstate, std::move(opts))) {
                throw JSONRPCError(RPC_MISC_ERROR, "Unable to import mempool file, see debug.log for details.");
            }

            UniValue ret{UniValue::VOBJ};
            return ret;
        },
    };
}

static RPCHelpMan savemempool()
{
    return RPCHelpMan{"savemempool",
        "\nDumps the mempool to disk. It will fail until the previous dump is fully loaded.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "filename", "the directory and file where the mempool was saved"},
            }},
        RPCExamples{
            HelpExampleCli("savemempool", "")
            + HelpExampleRpc("savemempool", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const ArgsManager& args{EnsureAnyArgsman(request.context)};
    const CTxMemPool& mempool = EnsureAnyMemPool(request.context);

    if (!mempool.GetLoadTried()) {
        throw JSONRPCError(RPC_MISC_ERROR, "The mempool was not loaded yet");
    }

    const fs::path& dump_path = MempoolPath(args);

    if (!DumpMempool(mempool, dump_path)) {
        throw JSONRPCError(RPC_MISC_ERROR, "Unable to dump mempool to disk");
    }

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("filename", dump_path.utf8string());

    return ret;
},
    };
}

static RPCHelpMan submitpackage()
{
    return RPCHelpMan{"submitpackage",
        "Submit a package of raw transactions (serialized, hex-encoded) to local node.\n"
        "The package will be validated according to consensus and mempool policy rules. If any transaction passes, it will be accepted to mempool.\n"
        "This RPC is experimental and the interface may be unstable. Refer to doc/policy/packages.md for documentation on package policies.\n"
        "Warning: successful submission does not mean the transactions will propagate throughout the network.\n"
        ,
        {
            {"package", RPCArg::Type::ARR, RPCArg::Optional::NO, "An array of raw transactions.\n"
                "The package must solely consist of a child and its parents. None of the parents may depend on each other.\n"
                "The package must be topologically sorted, with the child being the last element in the array.",
                {
                    {"rawtx", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, ""},
                },
            },
            {"maxfeerate", RPCArg::Type::AMOUNT, RPCArg::Default{FormatMoney(DEFAULT_MAX_RAW_TX_FEE_RATE.GetFeePerK())},
             "Reject transactions whose fee rate is higher than the specified value, expressed in " + CURRENCY_UNIT +
                 "/kvB.\nFee rates larger than 1BTC/kvB are rejected.\nSet to 0 to accept any fee rate."},
            {"maxburnamount", RPCArg::Type::AMOUNT, RPCArg::Default{FormatMoney(DEFAULT_MAX_BURN_AMOUNT)},
             "Reject transactions with provably unspendable outputs (e.g. 'datacarrier' outputs that use the OP_RETURN opcode) greater than the specified value, expressed in " + CURRENCY_UNIT + ".\n"
             "If burning funds through unspendable outputs is desired, increase this value.\n"
             "This check is based on heuristics and does not guarantee spendability of outputs.\n"
            },
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "package_msg", "The transaction package result message. \"success\" indicates all transactions were accepted into or are already in the mempool."},
                {RPCResult::Type::OBJ_DYN, "tx-results", "transaction results keyed by wtxid",
                {
                    {RPCResult::Type::OBJ, "wtxid", "transaction wtxid", {
                        {RPCResult::Type::STR_HEX, "txid", "The transaction hash in hex"},
                        {RPCResult::Type::STR_HEX, "other-wtxid", /*optional=*/true, "The wtxid of a different transaction with the same txid but different witness found in the mempool. This means the submitted transaction was ignored."},
                        {RPCResult::Type::NUM, "vsize", /*optional=*/true, "Sigops-adjusted virtual transaction size."},
                        {RPCResult::Type::OBJ, "fees", /*optional=*/true, "Transaction fees", {
                            {RPCResult::Type::STR_AMOUNT, "base", "transaction fee in " + CURRENCY_UNIT},
                            {RPCResult::Type::STR_AMOUNT, "effective-feerate", /*optional=*/true, "if the transaction was not already in the mempool, the effective feerate in " + CURRENCY_UNIT + " per KvB. For example, the package feerate and/or feerate with modified fees from prioritisetransaction."},
                            {RPCResult::Type::ARR, "effective-includes", /*optional=*/true, "if effective-feerate is provided, the wtxids of the transactions whose fees and vsizes are included in effective-feerate.",
                                {{RPCResult::Type::STR_HEX, "", "transaction wtxid in hex"},
                            }},
                        }},
                        {RPCResult::Type::STR, "error", /*optional=*/true, "The transaction error string, if it was rejected by the mempool"},
                    }}
                }},
                {RPCResult::Type::ARR, "replaced-transactions", /*optional=*/true, "List of txids of replaced transactions",
                {
                    {RPCResult::Type::STR_HEX, "", "The transaction id"},
                }},
            },
        },
        RPCExamples{
            HelpExampleRpc("submitpackage", R"(["rawtx1", "rawtx2"])") +
            HelpExampleCli("submitpackage", R"('["rawtx1", "rawtx2"]')")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            const UniValue raw_transactions = request.params[0].get_array();
            if (raw_transactions.size() < 2 || raw_transactions.size() > MAX_PACKAGE_COUNT) {
                throw JSONRPCError(RPC_INVALID_PARAMETER,
                                   "Array must contain between 2 and " + ToString(MAX_PACKAGE_COUNT) + " transactions.");
            }

            // Fee check needs to be run with chainstate and package context
            const CFeeRate max_raw_tx_fee_rate{ParseFeeRate(self.Arg<UniValue>("maxfeerate"))};
            std::optional<CFeeRate> client_maxfeerate{max_raw_tx_fee_rate};
            // 0-value is special; it's mapped to no sanity check
            if (max_raw_tx_fee_rate == CFeeRate(0)) {
                client_maxfeerate = std::nullopt;
            }

            // Burn sanity check is run with no context
            const CAmount max_burn_amount = request.params[2].isNull() ? 0 : AmountFromValue(request.params[2]);

            std::vector<CTransactionRef> txns;
            txns.reserve(raw_transactions.size());
            for (const auto& rawtx : raw_transactions.getValues()) {
                CMutableTransaction mtx;
                if (!DecodeHexTx(mtx, rawtx.get_str())) {
                    throw JSONRPCError(RPC_DESERIALIZATION_ERROR,
                                       "TX decode failed: " + rawtx.get_str() + " Make sure the tx has at least one input.");
                }

                for (const auto& out : mtx.vout) {
                    if((out.scriptPubKey.IsUnspendable() || !out.scriptPubKey.HasValidOps()) && out.nValue > max_burn_amount) {
                        throw JSONRPCTransactionError(TransactionError::MAX_BURN_EXCEEDED);
                    }
                }

                txns.emplace_back(MakeTransactionRef(std::move(mtx)));
            }
            if (!IsChildWithParentsTree(txns)) {
                throw JSONRPCTransactionError(TransactionError::INVALID_PACKAGE, "package topology disallowed. not child-with-parents or parents depend on each other.");
            }

            NodeContext& node = EnsureAnyNodeContext(request.context);
            CTxMemPool& mempool = EnsureMemPool(node);
            Chainstate& chainstate = EnsureChainman(node).ActiveChainstate();
            const auto package_result = WITH_LOCK(::cs_main, return ProcessNewPackage(chainstate, mempool, txns, /*test_accept=*/ false, client_maxfeerate));

            std::string package_msg = "success";

            // First catch package-wide errors, continue if we can
            switch(package_result.m_state.GetResult()) {
                case PackageValidationResult::PCKG_RESULT_UNSET:
                {
                    // Belt-and-suspenders check; everything should be successful here
                    CHECK_NONFATAL(package_result.m_tx_results.size() == txns.size());
                    for (const auto& tx : txns) {
                        CHECK_NONFATAL(mempool.exists(GenTxid::Txid(tx->GetHash())));
                    }
                    break;
                }
                case PackageValidationResult::PCKG_MEMPOOL_ERROR:
                {
                    // This only happens with internal bug; user should stop and report
                    throw JSONRPCTransactionError(TransactionError::MEMPOOL_ERROR,
                        package_result.m_state.GetRejectReason());
                }
                case PackageValidationResult::PCKG_POLICY:
                case PackageValidationResult::PCKG_TX:
                {
                    // Package-wide error we want to return, but we also want to return individual responses
                    package_msg = package_result.m_state.ToString();
                    CHECK_NONFATAL(package_result.m_tx_results.size() == txns.size() ||
                            package_result.m_tx_results.empty());
                    break;
                }
            }

            size_t num_broadcast{0};
            for (const auto& tx : txns) {
                // We don't want to re-submit the txn for validation in BroadcastTransaction
                if (!mempool.exists(GenTxid::Txid(tx->GetHash()))) {
                    continue;
                }

                // We do not expect an error here; we are only broadcasting things already/still in mempool
                std::string err_string;
                const auto err = BroadcastTransaction(node, tx, err_string, /*max_tx_fee=*/0, /*relay=*/true, /*wait_callback=*/true);
                if (err != TransactionError::OK) {
                    throw JSONRPCTransactionError(err,
                        strprintf("transaction broadcast failed: %s (%d transactions were broadcast successfully)",
                            err_string, num_broadcast));
                }
                num_broadcast++;
            }

            UniValue rpc_result{UniValue::VOBJ};
            rpc_result.pushKV("package_msg", package_msg);
            UniValue tx_result_map{UniValue::VOBJ};
            std::set<uint256> replaced_txids;
            for (const auto& tx : txns) {
                UniValue result_inner{UniValue::VOBJ};
                result_inner.pushKV("txid", tx->GetHash().GetHex());
                auto it = package_result.m_tx_results.find(tx->GetWitnessHash());
                if (it == package_result.m_tx_results.end()) {
                    // No results, report error and continue
                    result_inner.pushKV("error", "unevaluated");
                    continue;
                }
                const auto& tx_result = it->second;
                switch(it->second.m_result_type) {
                case MempoolAcceptResult::ResultType::DIFFERENT_WITNESS:
                    result_inner.pushKV("other-wtxid", it->second.m_other_wtxid.value().GetHex());
                    break;
                case MempoolAcceptResult::ResultType::INVALID:
                    result_inner.pushKV("error", it->second.m_state.ToString());
                    break;
                case MempoolAcceptResult::ResultType::VALID:
                case MempoolAcceptResult::ResultType::MEMPOOL_ENTRY:
                    result_inner.pushKV("vsize", int64_t{it->second.m_vsize.value()});
                    UniValue fees(UniValue::VOBJ);
                    fees.pushKV("base", ValueFromAmount(it->second.m_base_fees.value()));
                    if (tx_result.m_result_type == MempoolAcceptResult::ResultType::VALID) {
                        // Effective feerate is not provided for MEMPOOL_ENTRY transactions even
                        // though modified fees is known, because it is unknown whether package
                        // feerate was used when it was originally submitted.
                        fees.pushKV("effective-feerate", ValueFromAmount(tx_result.m_effective_feerate.value().GetFeePerK()));
                        UniValue effective_includes_res(UniValue::VARR);
                        for (const auto& wtxid : tx_result.m_wtxids_fee_calculations.value()) {
                            effective_includes_res.push_back(wtxid.ToString());
                        }
                        fees.pushKV("effective-includes", std::move(effective_includes_res));
                    }
                    result_inner.pushKV("fees", std::move(fees));
                    for (const auto& ptx : it->second.m_replaced_transactions) {
                        replaced_txids.insert(ptx->GetHash());
                    }
                    break;
                }
                tx_result_map.pushKV(tx->GetWitnessHash().GetHex(), std::move(result_inner));
            }
            rpc_result.pushKV("tx-results", std::move(tx_result_map));
            UniValue replaced_list(UniValue::VARR);
            for (const uint256& hash : replaced_txids) replaced_list.push_back(hash.ToString());
            rpc_result.pushKV("replaced-transactions", std::move(replaced_list));
            return rpc_result;
        },
    };
}

void RegisterMempoolRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[]{
        {"rawtransactions", &sendrawtransaction},
        {"rawtransactions", &testmempoolaccept},
        {"blockchain", &getmempoolancestors},
        {"blockchain", &getmempooldescendants},
        {"blockchain", &getmempoolentry},
        {"blockchain", &gettxspendingprevout},
        {"blockchain", &getmempoolinfo},
        {"blockchain", &getrawmempool},
        {"blockchain", &importmempool},
        {"blockchain", &savemempool},
        {"rawtransactions", &submitpackage},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
