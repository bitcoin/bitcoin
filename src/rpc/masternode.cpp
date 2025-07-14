// Copyright (c) 2014-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <evo/assetlocktx.h>
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
#include <util/check.h>
#include <util/strencodings.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/rpc/util.h>

#ifdef ENABLE_WALLET
#include <wallet/spend.h>
#include <wallet/wallet.h>
#endif // ENABLE_WALLET

#include <fstream>
#include <iomanip>

using node::GetTransaction;
using node::NodeContext;
using node::ReadBlockFromDisk;
#ifdef ENABLE_WALLET
using wallet::CCoinControl;
using wallet::CoinType;
using wallet::COutput;
using wallet::CWallet;
using wallet::GetWalletForJSONRPCRequest;
#endif // ENABLE_WALLET

static RPCHelpMan masternode_connect()
{
    return RPCHelpMan{"masternode connect",
        "Connect to given masternode\n",
        {
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The address of the masternode to connect"},
            {"v2transport", RPCArg::Type::BOOL, RPCArg::DefaultHint{"set by -v2transport"}, "Attempt to connect using BIP324 v2 transport protocol"},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::string strAddress = request.params[0].get_str();

    std::optional<CService> addr{Lookup(strAddress, 0, false)};
    if (!addr.has_value()) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("Incorrect masternode address %s", strAddress));
    }

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    CConnman& connman = EnsureConnman(node);

    bool node_v2transport = connman.GetLocalServices() & NODE_P2P_V2;
    bool use_v2transport = request.params[1].isNull() ? node_v2transport : ParseBoolV(request.params[1], "v2transport");

    if (use_v2transport && !node_v2transport) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Error: Adding v2transport connections requires -v2transport init flag to be set.");
    }

    connman.OpenMasternodeConnection(CAddress(addr.value(), NODE_NETWORK), use_v2transport);
    if (!connman.IsConnected(CAddress(addr.value(), NODE_NETWORK), CConnman::AllNodes)) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("Couldn't connect to masternode %s", strAddress));
    }

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
    const std::shared_ptr<const CWallet> wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    // Find possible candidates
    CCoinControl coin_control(CoinType::ONLY_MASTERNODE_COLLATERAL);

    UniValue outputsArr(UniValue::VARR);
    for (const auto& out : WITH_LOCK(wallet->cs_wallet, return AvailableCoinsListUnspent(*wallet, &coin_control).coins)) {
        outputsArr.push_back(out.outpoint.ToStringShort());
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

    if (!node.mn_activeman) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "This node does not run an active masternode.");
    }

    UniValue mnObj(UniValue::VOBJ);
    // keep compatibility with legacy status for now (TODO: get deprecated/removed later)
    mnObj.pushKV("outpoint", node.mn_activeman->GetOutPoint().ToStringShort());
    mnObj.pushKV("service", node.mn_activeman->GetService().ToStringAddrPort());
    auto dmn = CHECK_NONFATAL(node.dmnman)->GetListAtChainTip().GetMN(node.mn_activeman->GetProTxHash());
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
            NONFATAL_UNREACHABLE();
        }
        strPayments = EncodeDestination(dest);
        if (payee->nOperatorReward != 0 && payee->pdmnState->scriptOperatorPayout != CScript()) {
            if (!ExtractDestination(payee->pdmnState->scriptOperatorPayout, dest)) {
                NONFATAL_UNREACHABLE();
            }
            strPayments += ", " + EncodeDestination(dest);
        }
    }
    if (govman.IsSuperblockTriggered(tip_mn_list, nBlockHeight)) {
        std::vector<CTxOut> voutSuperblock;
        if (!govman.GetSuperblockPayments(tip_mn_list, nBlockHeight, voutSuperblock)) {
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
            {"count", RPCArg::Type::NUM, RPCArg::Default{10}, "number of last winners to return"},
            {"filter", RPCArg::Type::STR, RPCArg::Default{""}, "filter for returned winners"},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);
    const CBlockIndex* pindexTip{nullptr};
    {
        LOCK(::cs_main);
        pindexTip = chainman.ActiveChain().Tip();
        if (!pindexTip) return NullUniValue;
    }

    int nCount = 10;
    std::string strFilter;

    if (!request.params[0].isNull()) {
        nCount = LocaleIndependentAtoi<int>(request.params[0].get_str());
    }

    if (!request.params[1].isNull()) {
        strFilter = request.params[1].get_str();
    }

    UniValue obj(UniValue::VOBJ);

    int nChainTipHeight = pindexTip->nHeight;
    int nStartHeight = std::max(nChainTipHeight - nCount, 1);

    const auto tip_mn_list = CHECK_NONFATAL(node.dmnman)->GetListAtChainTip();
    for (int h = nStartHeight; h <= nChainTipHeight; h++) {
        const CBlockIndex* pIndex = pindexTip->GetAncestor(h - 1);
        auto payee = node.dmnman->GetListForBlock(pIndex).GetMNPayee(pIndex);
        if (payee) {
            std::string strPayments = GetRequiredPaymentsString(*CHECK_NONFATAL(node.govman), tip_mn_list, h, payee);
            if (!strFilter.empty() && strPayments.find(strFilter) == std::string::npos) continue;
            obj.pushKV(strprintf("%d", h), strPayments);
        }
    }

    auto projection = node.dmnman->GetListForBlock(pindexTip).GetProjectedMNPayees(pindexTip, 20);
    for (size_t i = 0; i < projection.size(); i++) {
        int h = nChainTipHeight + 1 + i;
        std::string strPayments = GetRequiredPaymentsString(*node.govman, tip_mn_list, h, projection[i]);
        if (!strFilter.empty() && strPayments.find(strFilter) == std::string::npos) continue;
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
            {"blockhash", RPCArg::Type::STR_HEX, RPCArg::DefaultHint{"tip"}, "The hash of the starting block"},
            {"count", RPCArg::Type::NUM, RPCArg::Default{1}, "The number of blocks to return. Will return <count> previous blocks if <count> is negative. Both 1 and -1 correspond to the chain tip."},
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

    const CBlockIndex* pindex{nullptr};

    if (g_txindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }

    if (request.params[0].isNull()) {
        LOCK(::cs_main);
        pindex = chainman.ActiveChain().Tip();
    } else {
        LOCK(::cs_main);
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
            if (tx->IsPlatformTransfer()) {
                auto payload = GetTxPayload<CAssetUnlockPayload>(*tx);
                CHECK_NONFATAL(payload);
                nBlockFees += payload->getFee();
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
            LOCK(::cs_main);
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
            {"mode", RPCArg::Type::STR, RPCArg::DefaultHint{"json"}, "The mode to run list in"},
            {"filter", RPCArg::Type::STR, RPCArg::Default{""}, "Filter results. Partial match by outpoint by default in all modes, additional matches in some modes are also available"},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::string strMode = "json";
    std::string strFilter;

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

    UniValue obj(UniValue::VOBJ);

    const auto mnList = CHECK_NONFATAL(node.dmnman)->GetListAtChainTip();
    const auto dmnToStatus = [&](const auto& dmn) {
        if (dmn.pdmnState->IsBanned()) {
            return "POSE_BANNED";
        }
        return "ENABLED";
    };
    const auto dmnToLastPaidTime = [&](const auto& dmn) {
        if (dmn.pdmnState->nLastPaidHeight == 0) {
            return (int)0;
        }

        LOCK(::cs_main);
        const CBlockIndex* pindex = chainman.ActiveChain()[dmn.pdmnState->nLastPaidHeight];
        return (int)pindex->nTime;
    };

    const bool showRecentMnsOnly = strMode == "recent";
    const bool showEvoOnly = strMode == "evo";
    const int tipHeight = WITH_LOCK(::cs_main, return chainman.ActiveChain().Tip()->nHeight);
    mnList.ForEachMN(false, [&](auto& dmn) {
        if (showRecentMnsOnly && dmn.pdmnState->IsBanned()) {
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

        std::string strAddress{};
        if (strMode == "addr" || strMode == "full" || strMode == "info" || strMode == "json" || strMode == "recent" ||
            strMode == "evo") {
            for (const NetInfoEntry& entry : dmn.pdmnState->netInfo->GetEntries()) {
                strAddress += entry.ToStringAddrPort() + " ";
            }
            if (!strAddress.empty()) strAddress.pop_back(); // Remove trailing space
        }

        if (strMode == "addr") {
            if (!strFilter.empty() && strAddress.find(strFilter) == std::string::npos &&
                strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, strAddress);
        } else if (strMode == "full") {
            std::string strFull = strprintf("%s %d %s %s %s %s",
                                    PadString(dmnToStatus(dmn), 18),
                                    dmn.pdmnState->nPoSePenalty,
                                    payeeStr,
                                    PadString(ToString(dmnToLastPaidTime(dmn)), 10),
                                    PadString(ToString(dmn.pdmnState->nLastPaidHeight), 6),
                                    strAddress);
            if (!strFilter.empty() && strFull.find(strFilter) == std::string::npos &&
                strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, strFull);
        } else if (strMode == "info") {
            std::string strInfo = strprintf("%s %d %s %s",
                                    PadString(dmnToStatus(dmn), 18),
                                    dmn.pdmnState->nPoSePenalty,
                                    payeeStr,
                                    strAddress);
            if (!strFilter.empty() && strInfo.find(strFilter) == std::string::npos &&
                strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, strInfo);
        } else if (strMode == "json" || strMode == "recent" || strMode == "evo") {
            std::string strInfo = strprintf("%s %s %s %s %d %d %d %s %s %s %s",
                                    dmn.proTxHash.ToString(),
                                    strAddress,
                                    payeeStr,
                                    dmnToStatus(dmn),
                                    dmn.pdmnState->nPoSePenalty,
                                    dmnToLastPaidTime(dmn),
                                    dmn.pdmnState->nLastPaidHeight,
                                    EncodeDestination(PKHash(dmn.pdmnState->keyIDOwner)),
                                    EncodeDestination(PKHash(dmn.pdmnState->keyIDVoting)),
                                    collateralAddressStr,
                                    dmn.pdmnState->pubKeyOperator.ToString());
            if (!strFilter.empty() && strInfo.find(strFilter) == std::string::npos &&
                strOutpoint.find(strFilter) == std::string::npos) return;
            UniValue objMN(UniValue::VOBJ);
            objMN.pushKV("proTxHash", dmn.proTxHash.ToString());
            objMN.pushKV("address", dmn.pdmnState->netInfo->GetPrimary().ToStringAddrPort());
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
            if (!strFilter.empty() && strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, dmn.pdmnState->nLastPaidHeight);
        } else if (strMode == "lastpaidtime") {
            if (!strFilter.empty() && strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, dmnToLastPaidTime(dmn));
        } else if (strMode == "payee") {
            if (!strFilter.empty() && payeeStr.find(strFilter) == std::string::npos &&
                strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, payeeStr);
        } else if (strMode == "owneraddress") {
            if (!strFilter.empty() && strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, EncodeDestination(PKHash(dmn.pdmnState->keyIDOwner)));
        } else if (strMode == "pubkeyoperator") {
            if (!strFilter.empty() && strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, dmn.pdmnState->pubKeyOperator.ToString());
        } else if (strMode == "status") {
            std::string strStatus = dmnToStatus(dmn);
            if (!strFilter.empty() && strStatus.find(strFilter) == std::string::npos &&
                strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, strStatus);
        } else if (strMode == "votingaddress") {
            if (!strFilter.empty() && strOutpoint.find(strFilter) == std::string::npos) return;
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

#ifdef ENABLE_WALLET
Span<const CRPCCommand> GetWalletMasternodeRPCCommands()
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              actor (function)
  //  --------------------- -----------------------
    { "dash",               &masternode_outputs,       },
};
// clang-format on
    return commands;
}
#endif // ENABLE_WALLET

void RegisterMasternodeRPCCommands(CRPCTable &t)
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              actor (function)
  //  --------------------- -----------------------
    { "dash",               &masternode_help,          },
    { "dash",               &masternodelist_composite, },
    { "dash",               &masternodelist,           },
    { "dash",               &masternode_connect,       },
    { "dash",               &masternode_count,         },
    { "dash",               &masternode_status,        },
    { "dash",               &masternode_payments,      },
    { "dash",               &masternode_winners,       },
};
// clang-format on
    for (const auto& command : commands) {
        t.appendCommand(command.name, &command);
    }
}
