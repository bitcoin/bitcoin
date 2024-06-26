// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <evo/chainhelper.h>
#include <evo/deterministicmns.h>
#include <governance/classes.h>
#include <index/txindex.h>
#include <node/blockstorage.h>
#include <node/context.h>
#include <governance/governance.h>
#include <masternode/node.h>
#include <masternode/payments.h>
#include <net.h>
#include <netbase.h>
#include <rpc/blockchain.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <univalue.h>
#include <util/strencodings.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/rpcwallet.h>
#ifdef ENABLE_WALLET
#include <wallet/wallet.h>
#endif // ENABLE_WALLET

#include <fstream>
#include <iomanip>

static RPCHelpMan masternode_connect()
{
    return RPCHelpMan{"masternode connect",
        "Connect to given masternode\n",
        {
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The address of the masternode to connect"},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::string strAddress = request.params[0].get_str();

    CService addr;
    if (!Lookup(strAddress, addr, 0, false))
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("Incorrect masternode address %s", strAddress));

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    CConnman& connman = EnsureConnman(node);

    connman.OpenMasternodeConnection(CAddress(addr, NODE_NETWORK));
    if (!connman.IsConnected(CAddress(addr, NODE_NETWORK), CConnman::AllNodes))
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("Couldn't connect to masternode %s", strAddress));

    return "successfully connected";
},
    };
}

static RPCHelpMan masternode_count()
{
    return RPCHelpMan{"masternode count",
        "Get information about number of masternodes.\n",
        {},
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);

    auto mnList = node.dmnman->GetListAtChainTip();
    int total = mnList.GetAllMNsCount();
    int enabled = mnList.GetValidMNsCount();

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("total", total);
    obj.pushKV("enabled", enabled);

    int evo_total = mnList.GetAllEvoCount();
    int evo_enabled = mnList.GetValidEvoCount();

    UniValue evoObj(UniValue::VOBJ);
    evoObj.pushKV("total", evo_total);
    evoObj.pushKV("enabled", evo_enabled);

    UniValue regularObj(UniValue::VOBJ);
    regularObj.pushKV("total", total - evo_total);
    regularObj.pushKV("enabled", enabled - evo_enabled);

    UniValue detailedObj(UniValue::VOBJ);
    detailedObj.pushKV("regular", regularObj);
    detailedObj.pushKV("evo", evoObj);
    obj.pushKV("detailed", detailedObj);

    return obj;
},
    };
}

static UniValue GetNextMasternodeForPayment(const CChain& active_chain, CDeterministicMNManager& dmnman, int heightShift)
{
    const CBlockIndex *tip = WITH_LOCK(::cs_main, return active_chain.Tip());
    auto mnList = dmnman.GetListForBlock(tip);
    auto payees = mnList.GetProjectedMNPayees(tip, heightShift);
    if (payees.empty())
        return "unknown";
    auto payee = payees.back();
    CScript payeeScript = payee->pdmnState->scriptPayout;

    CTxDestination payeeDest;
    ExtractDestination(payeeScript, payeeDest);

    UniValue obj(UniValue::VOBJ);

    obj.pushKV("height",        mnList.GetHeight() + heightShift);
    obj.pushKV("IP:port",       payee->pdmnState->addr.ToString());
    obj.pushKV("proTxHash",     payee->proTxHash.ToString());
    obj.pushKV("outpoint",      payee->collateralOutpoint.ToStringShort());
    obj.pushKV("payee",         IsValidDestination(payeeDest) ? EncodeDestination(payeeDest) : "UNKNOWN");
    return obj;
}

static RPCHelpMan masternode_winner()
{
    return RPCHelpMan{"masternode winner",
        "Print info on next masternode winner to vote for\n",
        {},
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    if (!IsDeprecatedRPCEnabled("masternode_winner")) {
        throw std::runtime_error("DEPRECATED: set -deprecatedrpc=masternode_winner to enable it");
    }

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);
    CHECK_NONFATAL(node.dmnman);
    return GetNextMasternodeForPayment(chainman.ActiveChain(), *node.dmnman, 10);
},
    };
}

static RPCHelpMan masternode_current()
{
    return RPCHelpMan{"masternode current",
        "Print info on current masternode winner to be paid the next block (calculated locally)\n",
        {},
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    if (!IsDeprecatedRPCEnabled("masternode_current")) {
        throw std::runtime_error("DEPRECATED: set -deprecatedrpc=masternode_current to enable it");
    }

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);
    CHECK_NONFATAL(node.dmnman);
    return GetNextMasternodeForPayment(chainman.ActiveChain(), *node.dmnman, 1);
},
    };
}

#ifdef ENABLE_WALLET
static RPCHelpMan masternode_outputs()
{
    return RPCHelpMan{"masternode outputs",
        "Print masternode compatible outputs\n",
        {},
        RPCResult {
            RPCResult::Type::ARR, "", "A list of outpoints that can be/are used as masternode collaterals",
            {
                {RPCResult::Type::STR, "", "A (potential) masternode collateral"},
            }},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    // Find possible candidates
    std::vector<COutput> vPossibleCoins;
    CCoinControl coin_control;
    coin_control.nCoinType = CoinType::ONLY_MASTERNODE_COLLATERAL;
    {
        LOCK(wallet->cs_wallet);
        wallet->AvailableCoins(vPossibleCoins, &coin_control);
    }
    UniValue outputsArr(UniValue::VARR);
    for (const auto& out : vPossibleCoins) {
        outputsArr.push_back(out.GetInputCoin().outpoint.ToStringShort());
    }

    return outputsArr;
},
    };
}

#endif // ENABLE_WALLET

static RPCHelpMan masternode_status()
{
    return RPCHelpMan{"masternode status",
        "Print masternode status information\n",
        {},
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    CHECK_NONFATAL(node.dmnman);
    if (!node.mn_activeman) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "This node does not run an active masternode.");
    }

    UniValue mnObj(UniValue::VOBJ);
    // keep compatibility with legacy status for now (might get deprecated/removed later)
    mnObj.pushKV("outpoint", node.mn_activeman->GetOutPoint().ToStringShort());
    mnObj.pushKV("service", node.mn_activeman->GetService().ToString());
    CDeterministicMNCPtr dmn = node.dmnman->GetListAtChainTip().GetMN(node.mn_activeman->GetProTxHash());
    if (dmn) {
        mnObj.pushKV("proTxHash", dmn->proTxHash.ToString());
        mnObj.pushKV("type", std::string(GetMnType(dmn->nType).description));
        mnObj.pushKV("collateralHash", dmn->collateralOutpoint.hash.ToString());
        mnObj.pushKV("collateralIndex", (int)dmn->collateralOutpoint.n);
        mnObj.pushKV("dmnState", dmn->pdmnState->ToJson(dmn->nType));
    }
    mnObj.pushKV("state", node.mn_activeman->GetStateString());
    mnObj.pushKV("status", node.mn_activeman->GetStatus());

    return mnObj;
},
    };
}

static std::string GetRequiredPaymentsString(CGovernanceManager& govman, const CDeterministicMNList& tip_mn_list, int nBlockHeight, const CDeterministicMNCPtr &payee)
{
    std::string strPayments = "Unknown";
    if (payee) {
        CTxDestination dest;
        if (!ExtractDestination(payee->pdmnState->scriptPayout, dest)) {
            CHECK_NONFATAL(false);
        }
        strPayments = EncodeDestination(dest);
        if (payee->nOperatorReward != 0 && payee->pdmnState->scriptOperatorPayout != CScript()) {
            if (!ExtractDestination(payee->pdmnState->scriptOperatorPayout, dest)) {
                CHECK_NONFATAL(false);
            }
            strPayments += ", " + EncodeDestination(dest);
        }
    }
    if (CSuperblockManager::IsSuperblockTriggered(govman, tip_mn_list, nBlockHeight)) {
        std::vector<CTxOut> voutSuperblock;
        if (!CSuperblockManager::GetSuperblockPayments(govman, tip_mn_list, nBlockHeight, voutSuperblock)) {
            return strPayments + ", error";
        }
        std::string strSBPayees = "Unknown";
        for (const auto& txout : voutSuperblock) {
            CTxDestination dest;
            ExtractDestination(txout.scriptPubKey, dest);
            if (strSBPayees != "Unknown") {
                strSBPayees += ", " + EncodeDestination(dest);
            } else {
                strSBPayees = EncodeDestination(dest);
            }
        }
        strPayments += ", " + strSBPayees;
    }
    return strPayments;
}

static RPCHelpMan masternode_winners()
{
    return RPCHelpMan{"masternode winners",
        "Print list of masternode winners\n",
        {
            {"count", RPCArg::Type::NUM, /* default */ "", "number of last winners to return"},
            {"filter", RPCArg::Type::STR, /* default */ "", "filter for returned winners"},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);
    const CBlockIndex* pindexTip{nullptr};
    {
        LOCK(cs_main);
        pindexTip = chainman.ActiveChain().Tip();
        if (!pindexTip) return NullUniValue;
    }

    int nCount = 10;
    std::string strFilter = "";

    if (!request.params[0].isNull()) {
        nCount = LocaleIndependentAtoi<int>(request.params[0].get_str());
    }

    if (!request.params[1].isNull()) {
        strFilter = request.params[1].get_str();
    }

    UniValue obj(UniValue::VOBJ);

    int nChainTipHeight = pindexTip->nHeight;
    int nStartHeight = std::max(nChainTipHeight - nCount, 1);

    CHECK_NONFATAL(node.dmnman);
    CHECK_NONFATAL(node.govman);

    const auto tip_mn_list = node.dmnman->GetListAtChainTip();
    for (int h = nStartHeight; h <= nChainTipHeight; h++) {
        const CBlockIndex* pIndex = pindexTip->GetAncestor(h - 1);
        auto payee = node.dmnman->GetListForBlock(pIndex).GetMNPayee(pIndex);
        std::string strPayments = GetRequiredPaymentsString(*node.govman, tip_mn_list, h, payee);
        if (strFilter != "" && strPayments.find(strFilter) == std::string::npos) continue;
        obj.pushKV(strprintf("%d", h), strPayments);
    }

    auto projection = node.dmnman->GetListForBlock(pindexTip).GetProjectedMNPayees(pindexTip, 20);
    for (size_t i = 0; i < projection.size(); i++) {
        int h = nChainTipHeight + 1 + i;
        std::string strPayments = GetRequiredPaymentsString(*node.govman, tip_mn_list, h, projection[i]);
        if (strFilter != "" && strPayments.find(strFilter) == std::string::npos) continue;
        obj.pushKV(strprintf("%d", h), strPayments);
    }

    return obj;
},
    };
}

static RPCHelpMan masternode_payments()
{
    return RPCHelpMan{"masternode payments",
        "\nReturns an array of deterministic masternodes and their payments for the specified block\n",
        {
            {"blockhash", RPCArg::Type::STR_HEX, /* default */ "tip", "The hash of the starting block"},
            {"count", RPCArg::Type::NUM, /* default */ "1", "The number of blocks to return. Will return <count> previous blocks if <count> is negative. Both 1 and -1 correspond to the chain tip."},
        },
        RPCResult {
            RPCResult::Type::ARR, "", "Blocks",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::NUM, "height", "The height of the block"},
                    {RPCResult::Type::STR_HEX, "blockhash", "The hash of the block"},
                    {RPCResult::Type::NUM, "amount", "Amount received in this block by all masternodes"},
                    {RPCResult::Type::ARR, "masternodes", "Masternodes that received payments in this block",
                    {
                        {RPCResult::Type::STR_HEX, "proTxHash", "The hash of the corresponding ProRegTx"},
                        {RPCResult::Type::NUM, "amount", "Amount received by this masternode"},
                        {RPCResult::Type::ARR, "payees", "Payees who received a share of this payment",
                        {
                            {RPCResult::Type::STR, "address", "Payee address"},
                            {RPCResult::Type::STR_HEX, "script", "Payee scriptPubKey"},
                            {RPCResult::Type::NUM, "amount", "Amount received by this payee"},
                        }},
                    }},
                }},
            },
        },
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);

    CBlockIndex* pindex{nullptr};

    if (g_txindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }

    if (request.params[0].isNull()) {
        LOCK(cs_main);
        pindex = chainman.ActiveChain().Tip();
    } else {
        LOCK(cs_main);
        uint256 blockHash(ParseHashV(request.params[0], "blockhash"));
        pindex = chainman.m_blockman.LookupBlockIndex(blockHash);
        if (pindex == nullptr) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        }
    }

    int64_t nCount = request.params.size() > 1 ? ParseInt64V(request.params[1], "count") : 1;

    // A temporary vector which is used to sort results properly (there is no "reverse" in/for UniValue)
    std::vector<UniValue> vecPayments;

    CHECK_NONFATAL(node.chain_helper);
    CHECK_NONFATAL(node.dmnman);
    while (vecPayments.size() < uint64_t(std::abs(nCount)) && pindex != nullptr) {
        CBlock block;
        if (!ReadBlockFromDisk(block, pindex, Params().GetConsensus())) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Can't read block from disk");
        }

        // Note: we have to actually calculate block reward from scratch instead of simply querying coinbase vout
        // because miners might collect less coins than they potentially could and this would break our calculations.
        CAmount nBlockFees{0};
        const CTxMemPool& mempool = EnsureAnyMemPool(request.context);
        for (const auto& tx : block.vtx) {
            if (tx->IsCoinBase()) {
                continue;
            }
            CAmount nValueIn{0};
            for (const auto& txin : tx->vin) {
                uint256 blockHashTmp;
                CTransactionRef txPrev = GetTransaction(/* block_index */ nullptr, &mempool, txin.prevout.hash, Params().GetConsensus(), blockHashTmp);
                nValueIn += txPrev->vout[txin.prevout.n].nValue;
            }
            nBlockFees += nValueIn - tx->GetValueOut();
        }

        std::vector<CTxOut> voutMasternodePayments, voutDummy;
        CMutableTransaction dummyTx;
        CAmount blockSubsidy = GetBlockSubsidy(pindex, Params().GetConsensus());
        node.chain_helper->mn_payments->FillBlockPayments(dummyTx, pindex->pprev, blockSubsidy, nBlockFees, voutMasternodePayments, voutDummy);

        UniValue blockObj(UniValue::VOBJ);
        CAmount payedPerBlock{0};

        UniValue masternodeArr(UniValue::VARR);
        UniValue protxObj(UniValue::VOBJ);
        UniValue payeesArr(UniValue::VARR);
        CAmount payedPerMasternode{0};

        for (const auto& txout : voutMasternodePayments) {
            UniValue obj(UniValue::VOBJ);
            CTxDestination dest;
            ExtractDestination(txout.scriptPubKey, dest);
            obj.pushKV("address", EncodeDestination(dest));
            obj.pushKV("script", HexStr(txout.scriptPubKey));
            obj.pushKV("amount", txout.nValue);
            payedPerMasternode += txout.nValue;
            payeesArr.push_back(obj);
        }

        // NOTE: we use _previous_ block to find a payee for the current one
        const auto dmnPayee = node.dmnman->GetListForBlock(pindex->pprev).GetMNPayee(pindex->pprev);
        protxObj.pushKV("proTxHash", dmnPayee == nullptr ? "" : dmnPayee->proTxHash.ToString());
        protxObj.pushKV("amount", payedPerMasternode);
        protxObj.pushKV("payees", payeesArr);
        payedPerBlock += payedPerMasternode;
        masternodeArr.push_back(protxObj);

        blockObj.pushKV("height", pindex->nHeight);
        blockObj.pushKV("blockhash", pindex->GetBlockHash().ToString());
        blockObj.pushKV("amount", payedPerBlock);
        blockObj.pushKV("masternodes", masternodeArr);
        vecPayments.push_back(blockObj);

        if (nCount > 0) {
            LOCK(cs_main);
            pindex = chainman.ActiveChain().Next(pindex);
        } else {
            pindex = pindex->pprev;
        }
    }

    if (nCount < 0) {
        std::reverse(vecPayments.begin(), vecPayments.end());
    }

    UniValue paymentsArr(UniValue::VARR);
    for (const auto& payment : vecPayments) {
        paymentsArr.push_back(payment);
    }

    return paymentsArr;
},
    };
}

static RPCHelpMan masternode_help()
{
    return RPCHelpMan{"masternode",
        "Set of commands to execute masternode related actions\n"
        "\nAvailable commands:\n"
        "  count        - Get information about number of masternodes\n"
        "  current      - DEPRECATED Print info on current masternode winner to be paid the next block (calculated locally)\n"
#ifdef ENABLE_WALLET
        "  outputs      - Print masternode compatible outputs\n"
#endif // ENABLE_WALLET
        "  status       - Print masternode status information\n"
        "  list         - Print list of all known masternodes (see masternodelist for more info)\n"
        "  payments     - Return information about masternode payments in a mined block\n"
        "  winner       - DEPRECATED Print info on next masternode winner to vote for\n"
        "  winners      - Print list of masternode winners\n",
        {
            {"command", RPCArg::Type::STR, RPCArg::Optional::NO, "The command to execute"},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    throw JSONRPCError(RPC_INVALID_PARAMETER, "Must be a valid command");
},
    };
}

static RPCHelpMan masternodelist_helper(bool is_composite)
{
    // We need both composite and non-composite options because we support
    // both options 'masternodelist' and 'masternode list'
    return RPCHelpMan{is_composite ? "masternode list" : "masternodelist",
        "Get a list of masternodes in different modes. This call is identical to 'masternode list' call.\n"
        "Available modes:\n"
        "  addr           - Print ip address associated with a masternode (can be additionally filtered, partial match)\n"
        "  recent         - Print info in JSON format for active and recently banned masternodes (can be additionally filtered, partial match)\n"
        "  evo            - Print info in JSON format for EvoNodes only\n"
        "  full           - Print info in format 'status payee lastpaidtime lastpaidblock IP'\n"
        "                   (can be additionally filtered, partial match)\n"
        "  info           - Print info in format 'status payee IP'\n"
        "                   (can be additionally filtered, partial match)\n"
        "  json           - Print info in JSON format (can be additionally filtered, partial match)\n"
        "  lastpaidblock  - Print the last block height a node was paid on the network\n"
        "  lastpaidtime   - Print the last time a node was paid on the network\n"
        "  owneraddress   - Print the masternode owner Dash address\n"
        "  payee          - Print the masternode payout Dash address (can be additionally filtered,\n"
        "                   partial match)\n"
        "  pubKeyOperator - Print the masternode operator public key\n"
        "  status         - Print masternode status: ENABLED / POSE_BANNED\n"
        "                   (can be additionally filtered, partial match)\n"
        "  votingaddress  - Print the masternode voting Dash address\n",
        {
            {"mode", RPCArg::Type::STR, /* default */ "json", "The mode to run list in"},
            {"filter", RPCArg::Type::STR, /* default */ "", "Filter results. Partial match by outpoint by default in all modes, additional matches in some modes are also available"},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::string strMode = "json";
    std::string strFilter = "";

    if (!request.params[0].isNull()) strMode = request.params[0].get_str();
    if (!request.params[1].isNull()) strFilter = request.params[1].get_str();

    strMode = ToLower(strMode);

    if (
                strMode != "addr" && strMode != "full" && strMode != "info" && strMode != "json" &&
                strMode != "owneraddress" && strMode != "votingaddress" &&
                strMode != "lastpaidtime" && strMode != "lastpaidblock" &&
                strMode != "payee" && strMode != "pubkeyoperator" &&
                strMode != "status" && strMode != "recent" && strMode != "evo")
    {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("strMode %s not found", strMode));
    }

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);
    CHECK_NONFATAL(node.dmnman);

    UniValue obj(UniValue::VOBJ);

    const auto mnList = node.dmnman->GetListAtChainTip();
    const auto dmnToStatus = [&](const auto& dmn) {
        if (mnList.IsMNValid(dmn)) {
            return "ENABLED";
        }
        if (mnList.IsMNPoSeBanned(dmn)) {
            return "POSE_BANNED";
        }
        return "UNKNOWN";
    };
    const auto dmnToLastPaidTime = [&](const auto& dmn) {
        if (dmn.pdmnState->nLastPaidHeight == 0) {
            return (int)0;
        }

        LOCK(cs_main);
        const CBlockIndex* pindex = chainman.ActiveChain()[dmn.pdmnState->nLastPaidHeight];
        return (int)pindex->nTime;
    };

    const bool showRecentMnsOnly = strMode == "recent";
    const bool showEvoOnly = strMode == "evo";
    const int tipHeight = WITH_LOCK(cs_main, return chainman.ActiveChain().Tip()->nHeight);
    mnList.ForEachMN(false, [&](auto& dmn) {
        if (showRecentMnsOnly && mnList.IsMNPoSeBanned(dmn)) {
            if (tipHeight - dmn.pdmnState->GetBannedHeight() > Params().GetConsensus().nSuperblockCycle) {
                return;
            }
        }
        if (showEvoOnly && dmn.nType != MnType::Evo) {
            return;
        }

        std::string strOutpoint = dmn.collateralOutpoint.ToStringShort();
        Coin coin;
        std::string collateralAddressStr = "UNKNOWN";
        if (GetUTXOCoin(chainman.ActiveChainstate(), dmn.collateralOutpoint, coin)) {
            CTxDestination collateralDest;
            if (ExtractDestination(coin.out.scriptPubKey, collateralDest)) {
                collateralAddressStr = EncodeDestination(collateralDest);
            }
        }

        CScript payeeScript = dmn.pdmnState->scriptPayout;
        CTxDestination payeeDest;
        std::string payeeStr = "UNKNOWN";
        if (ExtractDestination(payeeScript, payeeDest)) {
            payeeStr = EncodeDestination(payeeDest);
        }

        if (strMode == "addr") {
            std::string strAddress = dmn.pdmnState->addr.ToString();
            if (strFilter !="" && strAddress.find(strFilter) == std::string::npos &&
                strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, strAddress);
        } else if (strMode == "full") {
            std::ostringstream streamFull;
            streamFull << std::setw(18) <<
                           dmnToStatus(dmn) << " " <<
                           dmn.pdmnState->nPoSePenalty << " " <<
                           payeeStr << " " << std::setw(10) <<
                           dmnToLastPaidTime(dmn) << " "  << std::setw(6) <<
                           dmn.pdmnState->nLastPaidHeight << " " <<
                           dmn.pdmnState->addr.ToString();
            std::string strFull = streamFull.str();
            if (strFilter !="" && strFull.find(strFilter) == std::string::npos &&
                strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, strFull);
        } else if (strMode == "info") {
            std::ostringstream streamInfo;
            streamInfo << std::setw(18) <<
                           dmnToStatus(dmn) << " " <<
                           dmn.pdmnState->nPoSePenalty << " " <<
                           payeeStr << " " <<
                           dmn.pdmnState->addr.ToString();
            std::string strInfo = streamInfo.str();
            if (strFilter !="" && strInfo.find(strFilter) == std::string::npos &&
                strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, strInfo);
        } else if (strMode == "json" || strMode == "recent" || strMode == "evo") {
            std::ostringstream streamInfo;
            streamInfo <<  dmn.proTxHash.ToString() << " " <<
                           dmn.pdmnState->addr.ToString() << " " <<
                           payeeStr << " " <<
                           dmnToStatus(dmn) << " " <<
                           dmn.pdmnState->nPoSePenalty << " " <<
                           dmnToLastPaidTime(dmn) << " " <<
                           dmn.pdmnState->nLastPaidHeight << " " <<
                           EncodeDestination(PKHash(dmn.pdmnState->keyIDOwner)) << " " <<
                           EncodeDestination(PKHash(dmn.pdmnState->keyIDVoting)) << " " <<
                           collateralAddressStr << " " <<
                           dmn.pdmnState->pubKeyOperator.ToString();
            std::string strInfo = streamInfo.str();
            if (strFilter !="" && strInfo.find(strFilter) == std::string::npos &&
                strOutpoint.find(strFilter) == std::string::npos) return;
            UniValue objMN(UniValue::VOBJ);
            objMN.pushKV("proTxHash", dmn.proTxHash.ToString());
            objMN.pushKV("address", dmn.pdmnState->addr.ToString());
            objMN.pushKV("payee", payeeStr);
            objMN.pushKV("status", dmnToStatus(dmn));
            objMN.pushKV("type", std::string(GetMnType(dmn.nType).description));
            if (dmn.nType == MnType::Evo) {
                objMN.pushKV("platformNodeID", dmn.pdmnState->platformNodeID.ToString());
                objMN.pushKV("platformP2PPort", dmn.pdmnState->platformP2PPort);
                objMN.pushKV("platformHTTPPort", dmn.pdmnState->platformHTTPPort);
            }
            objMN.pushKV("pospenaltyscore", dmn.pdmnState->nPoSePenalty);
            objMN.pushKV("consecutivePayments", dmn.pdmnState->nConsecutivePayments);
            objMN.pushKV("lastpaidtime", dmnToLastPaidTime(dmn));
            objMN.pushKV("lastpaidblock", dmn.pdmnState->nLastPaidHeight);
            objMN.pushKV("owneraddress", EncodeDestination(PKHash(dmn.pdmnState->keyIDOwner)));
            objMN.pushKV("votingaddress", EncodeDestination(PKHash(dmn.pdmnState->keyIDVoting)));
            objMN.pushKV("collateraladdress", collateralAddressStr);
            objMN.pushKV("pubkeyoperator", dmn.pdmnState->pubKeyOperator.ToString());
            obj.pushKV(strOutpoint, objMN);
        } else if (strMode == "lastpaidblock") {
            if (strFilter !="" && strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, dmn.pdmnState->nLastPaidHeight);
        } else if (strMode == "lastpaidtime") {
            if (strFilter !="" && strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, dmnToLastPaidTime(dmn));
        } else if (strMode == "payee") {
            if (strFilter !="" && payeeStr.find(strFilter) == std::string::npos &&
                strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, payeeStr);
        } else if (strMode == "owneraddress") {
            if (strFilter !="" && strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, EncodeDestination(PKHash(dmn.pdmnState->keyIDOwner)));
        } else if (strMode == "pubkeyoperator") {
            if (strFilter !="" && strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, dmn.pdmnState->pubKeyOperator.ToString());
        } else if (strMode == "status") {
            std::string strStatus = dmnToStatus(dmn);
            if (strFilter !="" && strStatus.find(strFilter) == std::string::npos &&
                strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, strStatus);
        } else if (strMode == "votingaddress") {
            if (strFilter !="" && strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, EncodeDestination(PKHash(dmn.pdmnState->keyIDVoting)));
        }
    });

    return obj;
},
    };
}

static RPCHelpMan masternodelist()
{
    return masternodelist_helper(false);
}

static RPCHelpMan masternodelist_composite()
{
    return masternodelist_helper(true);
}

void RegisterMasternodeRPCCommands(CRPCTable &t)
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         argNames
  //  --------------------- ------------------------  -----------------------  ----------
    { "dash",               "masternode",             &masternode_help,          {"command"} },
    { "dash",               "masternode", "list",     &masternodelist_composite, {"mode", "filter"} },
    { "dash",               "masternodelist",         &masternodelist,           {"mode", "filter"} },
    { "dash",               "masternode", "connect",  &masternode_connect,       {"address"} },
    { "dash",               "masternode", "count",    &masternode_count,         {} },
#ifdef ENABLE_WALLET
    { "dash",               "masternode", "outputs",  &masternode_outputs,       {} },
#endif // ENABLE_WALLET
    { "dash",               "masternode", "status",   &masternode_status,        {} },
    { "dash",               "masternode", "payments", &masternode_payments,      {"blockhash", "count"} },
    { "dash",               "masternode", "winners",  &masternode_winners,       {"count", "filter"} },
    { "dash",               "masternode", "current",  &masternode_current,       {} },
    { "dash",               "masternode", "winner",   &masternode_winner,        {} },
};
// clang-format on
    for (const auto& command : commands) {
        t.appendCommand(command.name, command.subname, &command);
    }
}
