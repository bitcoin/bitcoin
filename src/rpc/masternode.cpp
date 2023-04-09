// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <evo/deterministicmns.h>
#include <governance/classes.h>
#include <index/txindex.h>
#include <node/context.h>
#include <governance/governance.h>
#include <masternode/node.h>
#include <masternode/payments.h>
#include <net.h>
#include <netbase.h>
#include <rpc/blockchain.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <univalue.h>
#include <util/strencodings.h>
#include <spork.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/rpcwallet.h>
#ifdef ENABLE_WALLET
#include <wallet/wallet.h>
#endif // ENABLE_WALLET

#include <fstream>
#include <iomanip>

static UniValue masternodelist(const JSONRPCRequest& request);

static void masternode_list_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"masternodelist",
        "Get a list of masternodes in different modes. This call is identical to 'masternode list' call.\n"
        "Available modes:\n"
        "  addr           - Print ip address associated with a masternode (can be additionally filtered, partial match)\n"
        "  recent         - Print info in JSON format for active and recently banned masternodes (can be additionally filtered, partial match)\n"
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
    }.Check(request);
}

static void masternode_connect_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"masternode connect",
        "Connect to given masternode\n",
        {
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The address of the masternode to connect"},
        },
        RPCResults{},
        RPCExamples{""}
    }.Check(request);
}

static UniValue masternode_connect(const JSONRPCRequest& request)
{
    masternode_connect_help(request);

    std::string strAddress = request.params[0].get_str();

    CService addr;
    if (!Lookup(strAddress, addr, 0, false))
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("Incorrect masternode address %s", strAddress));

    NodeContext& node = EnsureNodeContext(request.context);
    node.connman->OpenMasternodeConnection(CAddress(addr, NODE_NETWORK));
    if (!node.connman->IsConnected(CAddress(addr, NODE_NETWORK), CConnman::AllNodes))
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("Couldn't connect to masternode %s", strAddress));

    return "successfully connected";
}

static void masternode_count_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"masternode count",
        "Get information about number of masternodes.\n",
        {},
        RPCResults{},
        RPCExamples{""}
    }.Check(request);
}

static UniValue masternode_count(const JSONRPCRequest& request)
{
    masternode_count_help(request);

    auto mnList = deterministicMNManager->GetListAtChainTip();
    int total = mnList.GetAllMNsCount();
    int enabled = mnList.GetValidMNsCount();

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("total", total);
    obj.pushKV("enabled", enabled);

    int hpmn_total = mnList.GetAllHPMNsCount();
    int hpmn_enabled = mnList.GetValidHPMNsCount();

    UniValue hpmnObj(UniValue::VOBJ);
    hpmnObj.pushKV("total", hpmn_total);
    hpmnObj.pushKV("enabled", hpmn_enabled);

    UniValue regularObj(UniValue::VOBJ);
    regularObj.pushKV("total", total - hpmn_total);
    regularObj.pushKV("enabled", enabled - hpmn_enabled);

    UniValue detailedObj(UniValue::VOBJ);
    detailedObj.pushKV("regular", regularObj);
    detailedObj.pushKV("hpmn", hpmnObj);
    obj.pushKV("detailed", detailedObj);

    return obj;
}

static UniValue GetNextMasternodeForPayment(int heightShift)
{
    auto mnList = deterministicMNManager->GetListAtChainTip();
    auto payees = mnList.GetProjectedMNPayees(heightShift);
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

static void masternode_winner_help(const JSONRPCRequest& request)
{
    if (!IsDeprecatedRPCEnabled("masternode_winner")) {
        throw std::runtime_error("DEPRECATED: set -deprecatedrpc=masternode_winner to enable it");
    }

    RPCHelpMan{"masternode winner",
        "Print info on next masternode winner to vote for\n",
        {},
        RPCResults{},
        RPCExamples{""}
    }.Check(request);
}

static UniValue masternode_winner(const JSONRPCRequest& request)
{
    masternode_winner_help(request);
    return GetNextMasternodeForPayment(10);
}

static void masternode_current_help(const JSONRPCRequest& request)
{
    if (!IsDeprecatedRPCEnabled("masternode_current")) {
        throw std::runtime_error("DEPRECATED: set -deprecatedrpc=masternode_current to enable it");
    }

    RPCHelpMan{"masternode current",
        "Print info on current masternode winner to be paid the next block (calculated locally)\n",
        {},
        RPCResults{},
        RPCExamples{""}
    }.Check(request);
}

static UniValue masternode_current(const JSONRPCRequest& request)
{
    masternode_current_help(request);
    return GetNextMasternodeForPayment(1);
}

#ifdef ENABLE_WALLET
static void masternode_outputs_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"masternode outputs",
        "Print masternode compatible outputs\n",
        {},
        RPCResult {
            RPCResult::Type::ARR, "", "A list of outpoints that can be/are used as masternode collaterals",
            {
                {RPCResult::Type::STR, "", "A (potential) masternode collateral"},
            }},
        RPCExamples{""}
    }.Check(request);
}

static UniValue masternode_outputs(const JSONRPCRequest& request)
{
    masternode_outputs_help(request);

    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    // Find possible candidates
    std::vector<COutput> vPossibleCoins;
    CCoinControl coin_control;
    coin_control.nCoinType = CoinType::ONLY_MASTERNODE_COLLATERAL;
    {
        LOCK(wallet->cs_wallet);
        wallet->AvailableCoins(vPossibleCoins, true, &coin_control);
    }
    UniValue outputsArr(UniValue::VARR);
    for (const auto& out : vPossibleCoins) {
        outputsArr.push_back(out.GetInputCoin().outpoint.ToStringShort());
    }

    return outputsArr;
}

#endif // ENABLE_WALLET

static void masternode_status_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"masternode status",
        "Print masternode status information\n",
        {},
        RPCResults{},
        RPCExamples{""}
    }.Check(request);
}

static UniValue masternode_status(const JSONRPCRequest& request)
{
    masternode_status_help(request);

    if (!fMasternodeMode)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "This is not a masternode");

    UniValue mnObj(UniValue::VOBJ);

    CDeterministicMNCPtr dmn;
    {
        LOCK(activeMasternodeInfoCs);

        // keep compatibility with legacy status for now (might get deprecated/removed later)
        mnObj.pushKV("outpoint", activeMasternodeInfo.outpoint.ToStringShort());
        mnObj.pushKV("service", activeMasternodeInfo.service.ToString());
        dmn = deterministicMNManager->GetListAtChainTip().GetMN(activeMasternodeInfo.proTxHash);
    }
    if (dmn) {
        mnObj.pushKV("proTxHash", dmn->proTxHash.ToString());
        mnObj.pushKV("type", std::string(GetMnType(dmn->nType).description));
        mnObj.pushKV("collateralHash", dmn->collateralOutpoint.hash.ToString());
        mnObj.pushKV("collateralIndex", (int)dmn->collateralOutpoint.n);
        UniValue stateObj;
        dmn->pdmnState->ToJson(stateObj, dmn->nType);
        mnObj.pushKV("dmnState", stateObj);
    }
    mnObj.pushKV("state", activeMasternodeManager->GetStateString());
    mnObj.pushKV("status", activeMasternodeManager->GetStatus());

    return mnObj;
}

static std::string GetRequiredPaymentsString(int nBlockHeight, const CDeterministicMNCPtr &payee)
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
    if (CSuperblockManager::IsSuperblockTriggered(*governance, nBlockHeight)) {
        std::vector<CTxOut> voutSuperblock;
        if (!CSuperblockManager::GetSuperblockPayments(*governance, nBlockHeight, voutSuperblock)) {
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

static void masternode_winners_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"masternode winners",
        "Print list of masternode winners\n",
        {
            {"count", RPCArg::Type::NUM, /* default */ "", "number of last winners to return"},
            {"filter", RPCArg::Type::STR, /* default */ "", "filter for returned winners"},
        },
        RPCResults{},
        RPCExamples{""}
    }.Check(request);
}

static UniValue masternode_winners(const JSONRPCRequest& request)
{
    masternode_winners_help(request);

    const CBlockIndex* pindexTip{nullptr};
    {
        LOCK(cs_main);
        pindexTip = ::ChainActive().Tip();
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

    for (int h = nStartHeight; h <= nChainTipHeight; h++) {
        const CBlockIndex* pIndex = pindexTip->GetAncestor(h - 1);
        auto payee = deterministicMNManager->GetListForBlock(pIndex).GetMNPayee(pIndex);
        std::string strPayments = GetRequiredPaymentsString(h, payee);
        if (strFilter != "" && strPayments.find(strFilter) == std::string::npos) continue;
        obj.pushKV(strprintf("%d", h), strPayments);
    }

    auto projection = deterministicMNManager->GetListForBlock(pindexTip).GetProjectedMNPayees(20);
    for (size_t i = 0; i < projection.size(); i++) {
        int h = nChainTipHeight + 1 + i;
        std::string strPayments = GetRequiredPaymentsString(h, projection[i]);
        if (strFilter != "" && strPayments.find(strFilter) == std::string::npos) continue;
        obj.pushKV(strprintf("%d", h), strPayments);
    }

    return obj;
}
static void masternode_payments_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"masternode payments",
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
        RPCExamples{""}
    }.Check(request);
}

static UniValue masternode_payments(const JSONRPCRequest& request)
{
    masternode_payments_help(request);

    CBlockIndex* pindex{nullptr};

    if (g_txindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }

    if (request.params[0].isNull()) {
        LOCK(cs_main);
        pindex = ::ChainActive().Tip();
    } else {
        LOCK(cs_main);
        uint256 blockHash(ParseHashV(request.params[0], "blockhash"));
        pindex = g_chainman.m_blockman.LookupBlockIndex(blockHash);
        if (pindex == nullptr) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        }
    }

    int64_t nCount = request.params.size() > 1 ? ParseInt64V(request.params[1], "count") : 1;

    // A temporary vector which is used to sort results properly (there is no "reverse" in/for UniValue)
    std::vector<UniValue> vecPayments;

    while (vecPayments.size() < uint64_t(std::abs(nCount)) && pindex != nullptr) {

        CBlock block;
        if (!ReadBlockFromDisk(block, pindex, Params().GetConsensus())) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Can't read block from disk");
        }

        // Note: we have to actually calculate block reward from scratch instead of simply querying coinbase vout
        // because miners might collect less coins than they potentially could and this would break our calculations.
        CAmount nBlockFees{0};
        const CTxMemPool& mempool = EnsureMemPool(request.context);
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
        CAmount blockReward = nBlockFees + GetBlockSubsidy(pindex->pprev->nBits, pindex->pprev->nHeight, Params().GetConsensus());
        FillBlockPayments(*sporkManager, *governance, dummyTx, pindex->nHeight, blockReward, voutMasternodePayments, voutDummy);

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
        const auto dmnPayee = deterministicMNManager->GetListForBlock(pindex->pprev).GetMNPayee(pindex->pprev);
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
            pindex = ::ChainActive().Next(pindex);
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
}

[[ noreturn ]] static void masternode_help()
{
    RPCHelpMan{"masternode",
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
    }.Throw();
}

static UniValue masternode(const JSONRPCRequest& request)
{
    const JSONRPCRequest new_request{request.strMethod == "masternode" ? request.squashed() : request};
    const std::string command{new_request.strMethod};

    if (command == "masternodeconnect") {
        return masternode_connect(new_request);
    } else if (command == "masternodecount") {
        return masternode_count(new_request);
    } else if (command == "masternodecurrent") {
        return masternode_current(new_request);
    } else if (command == "masternodewinner") {
        return masternode_winner(new_request);
#ifdef ENABLE_WALLET
    } else if (command == "masternodeoutputs") {
        return masternode_outputs(new_request);
#endif // ENABLE_WALLET
    } else if (command == "masternodestatus") {
        return masternode_status(new_request);
    } else if (command == "masternodepayments") {
        return masternode_payments(new_request);
    } else if (command == "masternodewinners") {
        return masternode_winners(new_request);
    } else if (command == "masternodelist") {
        return masternodelist(new_request);
    } else {
        masternode_help();
    }
}

static UniValue masternodelist(const JSONRPCRequest& request)
{
    std::string strMode = "json";
    std::string strFilter = "";

    if (!request.params[0].isNull()) strMode = request.params[0].get_str();
    if (!request.params[1].isNull()) strFilter = request.params[1].get_str();

    strMode = ToLower(strMode);

    if (request.fHelp || (
                strMode != "addr" && strMode != "full" && strMode != "info" && strMode != "json" &&
                strMode != "owneraddress" && strMode != "votingaddress" &&
                strMode != "lastpaidtime" && strMode != "lastpaidblock" &&
                strMode != "payee" && strMode != "pubkeyoperator" &&
                strMode != "status" && strMode != "recent"))
    {
        masternode_list_help(request);
    }

    UniValue obj(UniValue::VOBJ);

    auto mnList = deterministicMNManager->GetListAtChainTip();
    auto dmnToStatus = [&](auto& dmn) {
        if (mnList.IsMNValid(dmn)) {
            return "ENABLED";
        }
        if (mnList.IsMNPoSeBanned(dmn)) {
            return "POSE_BANNED";
        }
        return "UNKNOWN";
    };
    auto dmnToLastPaidTime = [&](auto& dmn) {
        if (dmn.pdmnState->nLastPaidHeight == 0) {
            return (int)0;
        }

        LOCK(cs_main);
        const CBlockIndex* pindex = ::ChainActive()[dmn.pdmnState->nLastPaidHeight];
        return (int)pindex->nTime;
    };

    bool showRecentMnsOnly = strMode == "recent";
    int tipHeight = WITH_LOCK(cs_main, return ::ChainActive().Tip()->nHeight);
    mnList.ForEachMN(false, [&](auto& dmn) {
        if (showRecentMnsOnly && mnList.IsMNPoSeBanned(dmn)) {
            if (tipHeight - dmn.pdmnState->GetBannedHeight() > Params().GetConsensus().nSuperblockCycle) {
                return;
            }
        }

        std::string strOutpoint = dmn.collateralOutpoint.ToStringShort();
        Coin coin;
        std::string collateralAddressStr = "UNKNOWN";
        if (GetUTXOCoin(dmn.collateralOutpoint, coin)) {
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
            std::string strAddress = dmn.pdmnState->addr.ToString(false);
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
        } else if (strMode == "json" || strMode == "recent") {
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
                           dmn.pdmnState->pubKeyOperator.Get().ToString();
            std::string strInfo = streamInfo.str();
            if (strFilter !="" && strInfo.find(strFilter) == std::string::npos &&
                strOutpoint.find(strFilter) == std::string::npos) return;
            UniValue objMN(UniValue::VOBJ);
            objMN.pushKV("proTxHash", dmn.proTxHash.ToString());
            objMN.pushKV("address", dmn.pdmnState->addr.ToString());
            objMN.pushKV("payee", payeeStr);
            objMN.pushKV("status", dmnToStatus(dmn));
            objMN.pushKV("type", std::string(GetMnType(dmn.nType).description));
            if (dmn.nType == MnType::HighPerformance) {
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
            objMN.pushKV("pubkeyoperator", dmn.pdmnState->pubKeyOperator.Get().ToString());
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
            obj.pushKV(strOutpoint, dmn.pdmnState->pubKeyOperator.Get().ToString());
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
}
// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         argNames
  //  --------------------- ------------------------  -----------------------  ----------
    { "dash",               "masternode",             &masternode,             {} },
    { "dash",               "masternodelist",         &masternode,             {} },
};
// clang-format on
void RegisterMasternodeRPCCommands(CRPCTable &t)
{
    for (const auto& command : commands) {
        t.appendCommand(command.name, &command);
    }
}
