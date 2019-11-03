// Copyright (c) 2014-2019 The Dash Core developers
// Copyright (c) 2019 The BitGreen Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <base58.h>
#include <clientversion.h>
#include <init.h>
#include <key_io.h>
#include <masternodes/activemasternode.h>
#include <masternodes/payments.h>
#include <masternodes/sync.h>
#include <netbase.h>
#include <rpc/protocol.h>
#include <rpc/server.h>
#include <script/standard.h>
#include <txmempool.h>
#include <util/init.h>
#include <util/moneystr.h>
#include <validation.h>
#include <wallet/wallet.h>
#include <wallet/rpcwallet.h>

#include <special/deterministicmns.h>
#include <special/specialtx.h>
#include <special/util.h>

#include <fstream>
#include <iomanip>
#include <univalue.h>

UniValue masternodelist(const JSONRPCRequest& request);

void masternode_list_help()
{
    throw std::runtime_error(
        "masternode list ( \"mode\" \"filter\" )\n"
        "Get a list of masternodes in different modes. This call is identical to masternodelist call.\n"
        "\nArguments:\n"
        "1. \"mode\"      (string, optional/required to use filter, defaults = json) The mode to run list in\n"
        "2. \"filter\"    (string, optional) Filter results. Partial match by outpoint by default in all modes,\n"
        "                                    additional matches in some modes are also available\n"
        "\nAvailable modes:\n"
        "  addr           - Print ip address associated with a masternode (can be additionally filtered, partial match)\n"
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
        "  votingaddress  - Print the masternode voting Dash address\n");
}

UniValue masternode_list(const JSONRPCRequest& request)
{
    if (request.fHelp)
        masternode_list_help();
    JSONRPCRequest newRequest = request;
    newRequest.params.setArray();
    // forward params but skip "list"
    for (unsigned int i = 1; i < request.params.size(); i++) {
        newRequest.params.push_back(request.params[i]);
    }
    return masternodelist(newRequest);
}


void masternode_count_help()
{
    throw std::runtime_error(
        "masternode count (\"mode\")\n"
        "  Get information about number of masternodes. Mode\n"
        "  usage is depricated, call without mode params returns\n"
        "  all values in JSON format.\n"
        "\nArguments:\n"
        "1. \"mode\"      (string, optional, DEPRICATED) Option to get number of masternodes in different states\n"
        "\nAvailable modes:\n"
        "  total         - total number of masternodes"
        "  ps            - number of PrivateSend compatible masternodes"
        "  enabled       - number of enabled masternodes"
        "  qualify       - number of qualified masternodes"
        "  all           - all above in one string");
}

UniValue masternode_count(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 2)
        masternode_count_help();

    auto mnList = deterministicMNManager->GetListAtChainTip();
    int total = mnList.GetAllMNsCount();
    int enabled = mnList.GetValidMNsCount();

    if (request.params.size() == 1) {
        UniValue obj(UniValue::VOBJ);

        obj.pushKV("total", total);
        obj.pushKV("enabled", enabled);

        return obj;
    }

    std::string strMode = request.params[1].get_str();

    if (strMode == "total")
        return total;

    if (strMode == "enabled")
        return enabled;

    if (strMode == "all")
        return strprintf("Total: %d (Enabled: %d)",
            total, enabled);

    throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown mode value");
}

UniValue GetNextMasternodeForPayment(int heightShift)
{
    auto mnList = deterministicMNManager->GetListAtChainTip();
    auto payees = mnList.GetProjectedMNPayees(heightShift);
    if (payees.empty())
        return "unknown";
    auto payee = payees.back();
    CScript payeeScript = payee->pdmnState->scriptPayout;

    UniValue obj(UniValue::VOBJ);

    obj.pushKV("height", mnList.GetHeight() + heightShift);
    obj.pushKV("IP:port", payee->pdmnState->addr.ToString());
    obj.pushKV("proTxHash", payee->proTxHash.ToString());
    obj.pushKV("outpoint", payee->collateralOutpoint.ToString());

    CTxDestination dest;
    std::string payeeStr = "UNKNOWN";
    if (ExtractDestination(payeeScript, dest))
        payeeStr = EncodeDestination(dest);
    obj.pushKV("payee", payeeStr);

    return obj;
}

void masternode_winner_help()
{
    throw std::runtime_error(
        "masternode winner\n"
        "Print info on next masternode winner to vote for\n");
}

UniValue masternode_winner(const JSONRPCRequest& request)
{
    if (request.fHelp)
        masternode_winner_help();

    return GetNextMasternodeForPayment(10);
}

void masternode_current_help()
{
    throw std::runtime_error(
        "masternode current\n"
        "Print info on current masternode winner to be paid the next block (calculated locally)\n");
}

UniValue masternode_current(const JSONRPCRequest& request)
{
    if (request.fHelp)
        masternode_current_help();

    return GetNextMasternodeForPayment(1);
}

#ifdef ENABLE_WALLET
void masternode_outputs_help()
{
    throw std::runtime_error(
        "masternode outputs\n"
        "Print masternode compatible outputs\n");
}

UniValue masternode_outputs(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp)
        masternode_outputs_help();

    // Find possible candidates
    auto locked_chain = pwallet->chain().lock();
    std::vector<COutput> vPossibleCoins;
    // TODO: BitGreen - extract only UTXOs with 2500 BITGs
    pwallet->AvailableCoins(*locked_chain, vPossibleCoins, true); // , NULL, false, ONLY_COLLATERAL

    UniValue obj(UniValue::VOBJ);
    for (const auto& out : vPossibleCoins) {
        obj.pushKV(out.tx->GetHash().ToString(), strprintf("%d", out.i));
    }

    return obj;
}

#endif // ENABLE_WALLET

void masternode_status_help()
{
    throw std::runtime_error(
        "masternode status\n"
        "Print masternode status information\n");
}

UniValue masternode_status(const JSONRPCRequest& request)
{
    if (request.fHelp)
        masternode_status_help();

    if (!fMasternodeMode)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "This is not a masternode");

    UniValue mnObj(UniValue::VOBJ);

    // keep compatibility with legacy status for now (might get deprecated/removed later)
    mnObj.pushKV("outpoint", activeMasternodeInfo.outpoint.ToString());
    mnObj.pushKV("service", activeMasternodeInfo.service.ToString());

    auto dmn = activeMasternodeManager->GetDMN();
    if (dmn) {
        mnObj.pushKV("proTxHash", dmn->proTxHash.ToString());
        mnObj.pushKV("collateralHash", dmn->collateralOutpoint.hash.ToString());
        mnObj.pushKV("collateralIndex", (int)dmn->collateralOutpoint.n);
        UniValue stateObj;
        dmn->pdmnState->ToJson(stateObj);
        mnObj.pushKV("dmnState", stateObj);
    }
    mnObj.pushKV("state", activeMasternodeManager->GetStateString());
    mnObj.pushKV("status", activeMasternodeManager->GetStatus());

    return mnObj;
}

void masternode_winners_help()
{
    throw std::runtime_error(
        "masternode winners ( count \"filter\" )\n"
        "Print list of masternode winners\n"
        "\nArguments:\n"
        "1. count        (numeric, optional) number of last winners to return\n"
        "2. filter       (string, optional) filter for returned winners\n");
}

UniValue masternode_winners(const JSONRPCRequest& request)
{
    if (request.fHelp)
        masternode_winners_help();

    int nHeight;
    {
        LOCK(cs_main);
        CBlockIndex* pindex = ChainActive().Tip();
        if (!pindex) return NullUniValue;

        nHeight = pindex->nHeight;
    }

    int nLast = 10;
    std::string strFilter = "";

    if (request.params.size() >= 2) {
        nLast = atoi(request.params[1].get_str());
    }

    if (request.params.size() == 3) {
        strFilter = request.params[2].get_str();
    }

    if (request.params.size() > 3)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'masternode winners ( \"count\" \"filter\" )'");

    UniValue obj(UniValue::VOBJ);
    auto mapPayments = GetRequiredPaymentsStrings(nHeight - nLast, nHeight + 20);
    for (const auto& p : mapPayments) {
        obj.pushKV(strprintf("%d", p.first), p.second);
    }

    return obj;
}

[[noreturn]] void masternode_help() {
    throw std::runtime_error(
        "masternode \"command\"...\n"
        "Set of commands to execute masternode related actions\n"
        "\nArguments:\n"
        "1. \"command\"        (string or set of strings, required) The command to execute\n"
        "\nAvailable commands:\n"
        "  count        - Get information about number of masternodes (DEPRECATED options: 'total', 'ps', 'enabled', 'qualify', 'all')\n"
        "  current      - Print info on current masternode winner to be paid the next block (calculated locally)\n"
#ifdef ENABLE_WALLET
        "  outputs      - Print masternode compatible outputs\n"
#endif // ENABLE_WALLET
        "  status       - Print masternode status information\n"
        "  list         - Print list of all known masternodes (see masternodelist for more info)\n"
        "  winner       - Print info on next masternode winner to vote for\n"
        "  winners      - Print list of masternode winners\n");
}

UniValue masternode(const JSONRPCRequest& request)
{
    std::string strCommand;
    if (request.params.size() >= 1) {
        strCommand = request.params[0].get_str();
    }

#ifdef ENABLE_WALLET
    if (strCommand == "start-many")
        throw JSONRPCError(RPC_INVALID_PARAMETER, "DEPRECATED, please use start-all instead");
#endif // ENABLE_WALLET

    if (request.fHelp && strCommand.empty()) {
        masternode_help();
    }

    if (strCommand == "list") {
        return masternode_list(request);
    } else if (strCommand == "count") {
        return masternode_count(request);
    } else if (strCommand == "current") {
        return masternode_current(request);
    } else if (strCommand == "winner") {
        return masternode_winner(request);
#ifdef ENABLE_WALLET
    } else if (strCommand == "outputs") {
        return masternode_outputs(request);
#endif // ENABLE_WALLET
    } else if (strCommand == "status") {
        return masternode_status(request);
    } else if (strCommand == "winners") {
        return masternode_winners(request);
    } else {
        masternode_help();
    }
}

UniValue masternodelist(const JSONRPCRequest& request)
{
    std::string strMode = "json";
    std::string strFilter = "";

    if (request.params.size() >= 1) strMode = request.params[0].get_str();
    if (request.params.size() == 2) strFilter = request.params[1].get_str();

    std::transform(strMode.begin(), strMode.end(), strMode.begin(), ::tolower);

    if (request.fHelp || (strMode != "addr" && strMode != "full" && strMode != "info" && strMode != "json" &&
                             strMode != "owneraddress" && strMode != "votingaddress" &&
                             strMode != "lastpaidtime" && strMode != "lastpaidblock" &&
                             strMode != "payee" && strMode != "pubkeyoperator" &&
                             strMode != "status")) {
        masternode_list_help();
    }

    UniValue obj(UniValue::VOBJ);

    auto mnList = deterministicMNManager->GetListAtChainTip();
    auto dmnToStatus = [&](const CDeterministicMNCPtr& dmn) {
        if (mnList.IsMNValid(dmn)) {
            return "ENABLED";
        }
        if (mnList.IsMNPoSeBanned(dmn)) {
            return "POSE_BANNED";
        }
        return "UNKNOWN";
    };
    auto dmnToLastPaidTime = [&](const CDeterministicMNCPtr& dmn) {
        if (dmn->pdmnState->nLastPaidHeight == 0) {
            return (int)0;
        }

        LOCK(cs_main);
        const CBlockIndex* pindex = ChainActive()[dmn->pdmnState->nLastPaidHeight];
        return (int)pindex->nTime;
    };

    mnList.ForEachMN(false, [&](const CDeterministicMNCPtr& dmn) {
        std::string strOutpoint = dmn->collateralOutpoint.ToString();
        Coin coin;
        std::string collateralAddressStr = "UNKNOWN";
        if (GetUTXOCoin(dmn->collateralOutpoint, coin)) {
            CTxDestination collateralDest;
            if (ExtractDestination(coin.out.scriptPubKey, collateralDest)) {
                collateralAddressStr = EncodeDestination(collateralDest);
            }
        }

        CScript payeeScript = dmn->pdmnState->scriptPayout;
        CTxDestination payeeDest;
        std::string payeeStr = "UNKNOWN";
        if (ExtractDestination(payeeScript, payeeDest)) {
            payeeStr = EncodeDestination(payeeDest);
        }

        if (strMode == "addr") {
            std::string strAddress = dmn->pdmnState->addr.ToString();
            if (strFilter != "" && strAddress.find(strFilter) == std::string::npos &&
                strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, strAddress);
        } else if (strMode == "full") {
            std::ostringstream streamFull;
            streamFull << std::setw(18) << dmnToStatus(dmn) << " " << payeeStr << " " << std::setw(10) << dmnToLastPaidTime(dmn) << " " << std::setw(6) << dmn->pdmnState->nLastPaidHeight << " " << dmn->pdmnState->addr.ToString();
            std::string strFull = streamFull.str();
            if (strFilter != "" && strFull.find(strFilter) == std::string::npos &&
                strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, strFull);
        } else if (strMode == "info") {
            std::ostringstream streamInfo;
            streamInfo << std::setw(18) << dmnToStatus(dmn) << " " << payeeStr << " " << dmn->pdmnState->addr.ToString();
            std::string strInfo = streamInfo.str();
            if (strFilter != "" && strInfo.find(strFilter) == std::string::npos &&
                strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, strInfo);
        } else if (strMode == "json") {
            std::ostringstream streamInfo;
            streamInfo << dmn->proTxHash.ToString() << " " << dmn->pdmnState->addr.ToString() << " " << payeeStr << " " << dmnToStatus(dmn) << " " << dmnToLastPaidTime(dmn) << " " << dmn->pdmnState->nLastPaidHeight << " " << EncodeDestination(PKHash(dmn->pdmnState->keyIDOwner)) << " " << EncodeDestination(PKHash(dmn->pdmnState->keyIDVoting)) << " " << collateralAddressStr << " " << dmn->pdmnState->pubKeyOperator.Get().ToString();
            std::string strInfo = streamInfo.str();
            if (strFilter != "" && strInfo.find(strFilter) == std::string::npos &&
                strOutpoint.find(strFilter) == std::string::npos) return;
            UniValue objMN(UniValue::VOBJ);
            objMN.pushKV("proTxHash", dmn->proTxHash.ToString());
            objMN.pushKV("address", dmn->pdmnState->addr.ToString());
            objMN.pushKV("payee", payeeStr);
            objMN.pushKV("status", dmnToStatus(dmn));
            objMN.pushKV("lastpaidtime", dmnToLastPaidTime(dmn));
            objMN.pushKV("lastpaidblock", dmn->pdmnState->nLastPaidHeight);
            objMN.pushKV("owneraddress", EncodeDestination(PKHash(dmn->pdmnState->keyIDOwner)));
            objMN.pushKV("votingaddress", EncodeDestination(PKHash(dmn->pdmnState->keyIDVoting)));
            objMN.pushKV("collateraladdress", collateralAddressStr);
            objMN.pushKV("pubkeyoperator", dmn->pdmnState->pubKeyOperator.Get().ToString());
            obj.pushKV(strOutpoint, objMN);
        } else if (strMode == "lastpaidblock") {
            if (strFilter != "" && strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, dmn->pdmnState->nLastPaidHeight);
        } else if (strMode == "lastpaidtime") {
            if (strFilter != "" && strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, dmnToLastPaidTime(dmn));
        } else if (strMode == "payee") {
            if (strFilter != "" && payeeStr.find(strFilter) == std::string::npos &&
                strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, payeeStr);
        } else if (strMode == "owneraddress") {
            if (strFilter != "" && strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, EncodeDestination(PKHash(dmn->pdmnState->keyIDOwner)));
        } else if (strMode == "pubkeyoperator") {
            if (strFilter != "" && strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, dmn->pdmnState->pubKeyOperator.Get().ToString());
        } else if (strMode == "status") {
            std::string strStatus = dmnToStatus(dmn);
            if (strFilter != "" && strStatus.find(strFilter) == std::string::npos &&
                strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, strStatus);
        } else if (strMode == "votingaddress") {
            if (strFilter != "" && strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, EncodeDestination(PKHash(dmn->pdmnState->keyIDVoting)));
        }
    });

    return obj;
}

static const CRPCCommand commands[] =
    {
        //  category              name                      actor (function)          argNames
        //  --------------------- ------------------------  -----------------------   ----------
        {"bitgreen", "masternode", &masternode, {}},
        {"bitgreen", "masternodelist", &masternodelist, {}},
};

void RegisterMasternodeRPCCommands(CRPCTable& t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
