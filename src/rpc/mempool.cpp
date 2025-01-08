// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/blockchain.h>

#include <kernel/mempool_persist.h>

#include <chainparams.h>
#include <core_io.h>
#include <fs.h>
#include <node/mempool_persist_args.h>
#include <policy/rbf.h>
#include <policy/settings.h>
#include <primitives/transaction.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <txmempool.h>
#include <univalue.h>
#include <util/moneystr.h>
#include <util/time.h>

using kernel::DumpMempool;

using node::DEFAULT_MAX_RAW_TX_FEE_RATE;
using node::MempoolPath;
using node::NodeContext;

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
                 "/kvB.\nSet to 0 to accept any fee rate.\n"},
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
            RPCTypeCheck(request.params, {
                UniValue::VSTR,
                UniValueType(), // VNUM or VSTR, checked inside AmountFromValue()
            });

            CMutableTransaction mtx;
            if (!DecodeHexTx(mtx, request.params[0].get_str())) {
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed. Make sure the tx has at least one input.");
            }
            CTransactionRef tx(MakeTransactionRef(std::move(mtx)));

            const CFeeRate max_raw_tx_fee_rate = request.params[1].isNull() ?
                                                     DEFAULT_MAX_RAW_TX_FEE_RATE :
                                                     CFeeRate(AmountFromValue(request.params[1]));

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
             "Reject transactions whose fee rate is higher than the specified value, expressed in " + CURRENCY_UNIT + "/kvB\n"},
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
            RPCTypeCheck(request.params, {
                UniValue::VARR,
                UniValueType(), // VNUM or VSTR, checked inside AmountFromValue()
            });
            const UniValue raw_transactions = request.params[0].get_array();
            if (raw_transactions.size() < 1 || raw_transactions.size() > MAX_PACKAGE_COUNT) {
                throw JSONRPCError(RPC_INVALID_PARAMETER,
                                   "Array must contain between 1 and " + ToString(MAX_PACKAGE_COUNT) + " transactions.");
            }

            const CFeeRate max_raw_tx_fee_rate = request.params[1].isNull() ?
                                                     DEFAULT_MAX_RAW_TX_FEE_RATE :
                                                     CFeeRate(AmountFromValue(request.params[1]));

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
                if (txns.size() > 1) return ProcessNewPackage(chainstate, mempool, txns, /*test_accept=*/true);
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
                    result_inner.pushKV("package-error", package_result.m_state.GetRejectReason());
                }
                auto it = package_result.m_tx_results.find(tx->GetWitnessHash());
                if (exit_early || it == package_result.m_tx_results.end()) {
                    // Validation unfinished. Just return the txid and wtxid.
                    rpc_result.push_back(result_inner);
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
                        result_inner.pushKV("fees", fees);
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
                rpc_result.push_back(result_inner);
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

    info.pushKV("vsize", (int)e.GetTxSize());
    info.pushKV("weight", (int)e.GetTxWeight());
    info.pushKV("time", count_seconds(e.GetTime()));
    info.pushKV("height", (int)e.GetHeight());
    info.pushKV("descendantcount", e.GetCountWithDescendants());
    info.pushKV("descendantsize", e.GetSizeWithDescendants());
    info.pushKV("ancestorcount", e.GetCountWithAncestors());
    info.pushKV("ancestorsize", e.GetSizeWithAncestors());
    info.pushKV("wtxid", pool.vTxHashes[e.vTxHashesIdx].first.ToString());

    UniValue fees(UniValue::VOBJ);
    fees.pushKV("base", ValueFromAmount(e.GetFee()));
    fees.pushKV("modified", ValueFromAmount(e.GetModifiedFee()));
    fees.pushKV("ancestor", ValueFromAmount(e.GetModFeesWithAncestors()));
    fees.pushKV("descendant", ValueFromAmount(e.GetModFeesWithDescendants()));
    info.pushKV("fees", fees);

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

    info.pushKV("depends", depends);

    UniValue spent(UniValue::VARR);
    const CTxMemPool::txiter& it = pool.mapTx.find(tx.GetHash());
    const CTxMemPoolEntry::Children& children = it->GetMemPoolChildrenConst();
    for (const CTxMemPoolEntry& child : children) {
        spent.push_back(child.GetTx().GetHash().ToString());
    }

    info.pushKV("spentby", spent);

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
        for (const CTxMemPoolEntry& e : pool.mapTx) {
            const uint256& hash = e.GetTx().GetHash();
            UniValue info(UniValue::VOBJ);
            entryToJSON(pool, info, e);
            // Mempool has unique entries so there is no advantage in using
            // UniValue::pushKV, which checks if the key already exists in O(N).
            // UniValue::__pushKV is used instead which currently is O(1).
            o.__pushKV(hash.ToString(), info);
        }
        return o;
    } else {
        uint64_t mempool_sequence;
        std::vector<uint256> vtxid;
        {
            LOCK(pool.cs);
            pool.queryHashes(vtxid);
            mempool_sequence = pool.GetSequence();
        }
        UniValue a(UniValue::VARR);
        for (const uint256& hash : vtxid)
            a.push_back(hash.ToString());

        if (!include_mempool_sequence) {
            return a;
        } else {
            UniValue o(UniValue::VOBJ);
            o.pushKV("txids", a);
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

    CTxMemPool::txiter it = mempool.mapTx.find(hash);
    if (it == mempool.mapTx.end()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not in mempool");
    }

    CTxMemPool::setEntries setAncestors;
    uint64_t noLimit = std::numeric_limits<uint64_t>::max();
    std::string dummy;
    mempool.CalculateMemPoolAncestors(*it, setAncestors, noLimit, noLimit, noLimit, noLimit, dummy, false);

    if (!fVerbose) {
        UniValue o(UniValue::VARR);
        for (CTxMemPool::txiter ancestorIt : setAncestors) {
            o.push_back(ancestorIt->GetTx().GetHash().ToString());
        }
        return o;
    } else {
        UniValue o(UniValue::VOBJ);
        for (CTxMemPool::txiter ancestorIt : setAncestors) {
            const CTxMemPoolEntry &e = *ancestorIt;
            const uint256& _hash = e.GetTx().GetHash();
            UniValue info(UniValue::VOBJ);
            entryToJSON(mempool, info, e);
            o.pushKV(_hash.ToString(), info);
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

    CTxMemPool::txiter it = mempool.mapTx.find(hash);
    if (it == mempool.mapTx.end()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not in mempool");
    }

    CTxMemPool::setEntries setDescendants;
    mempool.CalculateDescendants(it, setDescendants);
    // CTxMemPool::CalculateDescendants will include the given tx
    setDescendants.erase(it);

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
            o.pushKV(_hash.ToString(), info);
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

    CTxMemPool::txiter it = mempool.mapTx.find(hash);
    if (it == mempool.mapTx.end()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not in mempool");
    }

    const CTxMemPoolEntry &e = *it;
    UniValue info(UniValue::VOBJ);
    entryToJSON(mempool, info, e);
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
            + HelpExampleRpc("gettxspendingprevout", "[{\"txid\":\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\",\"vout\":3}]")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            RPCTypeCheckArgument(request.params[0], UniValue::VARR);
            const UniValue& output_params = request.params[0];
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

                const uint256 txid(ParseHashO(o, "txid"));
                const int nOutput{find_value(o, "vout").getInt<int>()};
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

                result.push_back(o);
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
    ret.pushKV("maxmempool", pool.m_max_size_bytes);
    ret.pushKV("mempoolminfee", ValueFromAmount(std::max(pool.GetMinFee(), pool.m_min_relay_feerate).GetFeePerK()));
    ret.pushKV("minrelaytxfee", ValueFromAmount(pool.m_min_relay_feerate.GetFeePerK()));
    ret.pushKV("incrementalrelayfee", ValueFromAmount(pool.m_incremental_relay_feerate.GetFeePerK()));
    ret.pushKV("unbroadcastcount", uint64_t{pool.GetUnbroadcastTxs().size()});
    ret.pushKV("fullrbf", pool.m_full_rbf);
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
                {RPCResult::Type::BOOL, "loaded", "True if the mempool is fully loaded"},
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
    ret.pushKV("filename", dump_path.u8string());

    return ret;
},
    };
}

static RPCHelpMan submitpackage()
{
    return RPCHelpMan{"submitpackage",
        "Submit a package of raw transactions (serialized, hex-encoded) to local node (-regtest only).\n"
        "The package will be validated according to consensus and mempool policy rules. If all transactions pass, they will be accepted to mempool.\n"
        "This RPC is experimental and the interface may be unstable. Refer to doc/policy/packages.md for documentation on package policies.\n"
        "Warning: until package relay is in use, successful submission does not mean the transaction will propagate to other nodes on the network.\n"
        "Currently, each transaction is broadcasted individually after submission, which means they must meet other nodes' feerate requirements alone.\n"
        ,
        {
            {"package", RPCArg::Type::ARR, RPCArg::Optional::NO, "An array of raw transactions.",
                {
                    {"rawtx", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, ""},
                },
            },
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::OBJ_DYN, "tx-results", "transaction results keyed by wtxid",
                {
                    {RPCResult::Type::OBJ, "wtxid", "transaction wtxid", {
                        {RPCResult::Type::STR_HEX, "txid", "The transaction hash in hex"},
                        {RPCResult::Type::STR_HEX, "other-wtxid", /*optional=*/true, "The wtxid of a different transaction with the same txid but different witness found in the mempool. This means the submitted transaction was ignored."},
                        {RPCResult::Type::NUM, "vsize", "Virtual transaction size as defined in BIP 141."},
                        {RPCResult::Type::OBJ, "fees", "Transaction fees", {
                            {RPCResult::Type::STR_AMOUNT, "base", "transaction fee in " + CURRENCY_UNIT},
                        }},
                    }}
                }},
                {RPCResult::Type::STR_AMOUNT, "package-feerate", /*optional=*/true, "package feerate used for feerate checks in " + CURRENCY_UNIT + " per KvB. Excludes transactions which were deduplicated or accepted individually."},
                {RPCResult::Type::ARR, "replaced-transactions", /*optional=*/true, "List of txids of replaced transactions",
                {
                    {RPCResult::Type::STR_HEX, "", "The transaction id"},
                }},
            },
        },
        RPCExamples{
            HelpExampleCli("testmempoolaccept", "[rawtx1, rawtx2]") +
            HelpExampleCli("submitpackage", "[rawtx1, rawtx2]")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            if (!Params().IsMockableChain()) {
                throw std::runtime_error("submitpackage is for regression testing (-regtest mode) only");
            }
            RPCTypeCheck(request.params, {
                UniValue::VARR,
            });
            const UniValue raw_transactions = request.params[0].get_array();
            if (raw_transactions.size() < 1 || raw_transactions.size() > MAX_PACKAGE_COUNT) {
                throw JSONRPCError(RPC_INVALID_PARAMETER,
                                   "Array must contain between 1 and " + ToString(MAX_PACKAGE_COUNT) + " transactions.");
            }

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
            Chainstate& chainstate = EnsureChainman(node).ActiveChainstate();
            const auto package_result = WITH_LOCK(::cs_main, return ProcessNewPackage(chainstate, mempool, txns, /*test_accept=*/ false));

            // First catch any errors.
            switch(package_result.m_state.GetResult()) {
                case PackageValidationResult::PCKG_RESULT_UNSET: break;
                case PackageValidationResult::PCKG_POLICY:
                {
                    throw JSONRPCTransactionError(TransactionError::INVALID_PACKAGE,
                        package_result.m_state.GetRejectReason());
                }
                case PackageValidationResult::PCKG_MEMPOOL_ERROR:
                {
                    throw JSONRPCTransactionError(TransactionError::MEMPOOL_ERROR,
                        package_result.m_state.GetRejectReason());
                }
                case PackageValidationResult::PCKG_TX:
                {
                    for (const auto& tx : txns) {
                        auto it = package_result.m_tx_results.find(tx->GetWitnessHash());
                        if (it != package_result.m_tx_results.end() && it->second.m_state.IsInvalid()) {
                            throw JSONRPCTransactionError(TransactionError::MEMPOOL_REJECTED,
                                strprintf("%s failed: %s", tx->GetHash().ToString(), it->second.m_state.GetRejectReason()));
                        }
                    }
                    // If a PCKG_TX error was returned, there must have been an invalid transaction.
                    NONFATAL_UNREACHABLE();
                }
            }
            for (const auto& tx : txns) {
                size_t num_submitted{0};
                std::string err_string;
                const auto err = BroadcastTransaction(node, tx, err_string, 0, true, true);
                if (err != TransactionError::OK) {
                    throw JSONRPCTransactionError(err,
                        strprintf("transaction broadcast failed: %s (all transactions were submitted, %d transactions were broadcast successfully)",
                            err_string, num_submitted));
                }
            }
            UniValue rpc_result{UniValue::VOBJ};
            UniValue tx_result_map{UniValue::VOBJ};
            std::set<uint256> replaced_txids;
            for (const auto& tx : txns) {
                auto it = package_result.m_tx_results.find(tx->GetWitnessHash());
                CHECK_NONFATAL(it != package_result.m_tx_results.end());
                UniValue result_inner{UniValue::VOBJ};
                result_inner.pushKV("txid", tx->GetHash().GetHex());
                if (it->second.m_result_type == MempoolAcceptResult::ResultType::DIFFERENT_WITNESS) {
                    result_inner.pushKV("other-wtxid", it->second.m_other_wtxid.value().GetHex());
                }
                if (it->second.m_result_type == MempoolAcceptResult::ResultType::VALID ||
                    it->second.m_result_type == MempoolAcceptResult::ResultType::MEMPOOL_ENTRY) {
                    result_inner.pushKV("vsize", int64_t{it->second.m_vsize.value()});
                    UniValue fees(UniValue::VOBJ);
                    fees.pushKV("base", ValueFromAmount(it->second.m_base_fees.value()));
                    result_inner.pushKV("fees", fees);
                    if (it->second.m_replaced_transactions.has_value()) {
                        for (const auto& ptx : it->second.m_replaced_transactions.value()) {
                            replaced_txids.insert(ptx->GetHash());
                        }
                    }
                }
                tx_result_map.pushKV(tx->GetWitnessHash().GetHex(), result_inner);
            }
            rpc_result.pushKV("tx-results", tx_result_map);
            if (package_result.m_package_feerate.has_value()) {
                rpc_result.pushKV("package-feerate", ValueFromAmount(package_result.m_package_feerate.value().GetFeePerK()));
            }
            UniValue replaced_list(UniValue::VARR);
            for (const uint256& hash : replaced_txids) replaced_list.push_back(hash.ToString());
            rpc_result.pushKV("replaced-transactions", replaced_list);
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
        {"blockchain", &savemempool},
        {"hidden", &submitpackage},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
