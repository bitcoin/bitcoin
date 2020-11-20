// Copyright (c) 2014-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/activemasternode.h>
#include <netbase.h>
#include <validation.h>
#include <masternode/masternodepayments.h>
#include <rpc/server.h>
#include <rpc/blockchain.h>
#include <node/context.h>
RPCHelpMan masternodelist();

static RPCHelpMan masternode_list()
{
    return RPCHelpMan{"masternode_list",
        "\nGet a list of masternodes in different modes. This call is identical to 'masternode list' call\n",
        {
            {"mode", RPCArg::Type::STR, "json", "The mode to run list in.\n"
            "\nAvailable modes:\n"
            "  addr           - Print ip address associated with a masternode (can be additionally filtered, partial match)\n"
            "  full           - Print info in format 'status payee lastpaidtime lastpaidblock IP'\n"
            "                   (can be additionally filtered, partial match)\n"
            "  info           - Print info in format 'status payee IP'\n"
            "                   (can be additionally filtered, partial match)\n"
            "  json           - Print info in JSON format (can be additionally filtered, partial match)\n"
            "  lastpaidblock  - Print the last block height a node was paid on the network\n"
            "  lastpaidtime   - Print the last time a node was paid on the network\n"
            "  owneraddress   - Print the masternode owner Syscoin address\n"
            "  payee          - Print the masternode payout Syscoin address (can be additionally filtered,\n"
            "                   partial match)\n"
            "  pubKeyOperator - Print the masternode operator public key\n"
            "  status         - Print masternode status: ENABLED / POSE_BANNED\n"
            "                   (can be additionally filtered, partial match)\n"
            "  votingaddress  - Print the masternode voting Syscoin address\n"},     
            {"filter", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Filter results. Partial match by outpoint by default in all modes,\n"
                            "additional matches in some modes are also available.\n"},           
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{
                HelpExampleCli("masternode_list", "")
            + HelpExampleRpc("masternode_list", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    JSONRPCRequest newRequest = request;
    newRequest.params.setArray();
    for (unsigned int i = 0; i < request.params.size(); i++) {
        newRequest.params.push_back(request.params[i]);
    }
    return masternodelist().HandleRequest(newRequest);
},
    };
} 


static RPCHelpMan masternode_connect()
{
    return RPCHelpMan{"masternode_connect",
        "\nConnect to given masternode\n",
        {
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The address of the masternode to connect."},                
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{
                HelpExampleCli("masternode_connect", "")
            + HelpExampleRpc("masternode_connect", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::string strAddress = request.params[0].get_str();

    CService addr;
    if (!Lookup(strAddress.c_str(), addr, 0, false))
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("Incorrect masternode address %s", strAddress));
  NodeContext& node = EnsureNodeContext(request.context);
  if(!node.connman)
      throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");
    // TODO: Pass CConnman instance somehow and don't use global variable.
    node.connman->OpenMasternodeConnection(CAddress(addr, NODE_NETWORK));
    if (!node.connman->IsConnected(CAddress(addr, NODE_NETWORK), AllNodes))
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("Couldn't connect to masternode %s", strAddress));

    return "successfully connected";
},
    };
} 

static RPCHelpMan masternode_count()
{
    return RPCHelpMan{"masternode_count",
        "\nGet information about number of masternodes\n",
        {              
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{
                HelpExampleCli("masternode_count", "")
            + HelpExampleRpc("masternode_count", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    CDeterministicMNList mnList;
    if(deterministicMNManager)
        deterministicMNManager->GetListAtChainTip(mnList);
    int total = mnList.GetAllMNsCount();
    int enabled = mnList.GetValidMNsCount();

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("total", total);
    obj.pushKV("enabled", enabled);
    return obj;
},
    };
} 

UniValue GetNextMasternodeForPayment(size_t heightShift)
{
    CDeterministicMNList mnList;
    if(deterministicMNManager)
        deterministicMNManager->GetListAtChainTip(mnList);
    std::vector<CDeterministicMNCPtr> payees;
    mnList.GetProjectedMNPayees(heightShift, payees);
    if (payees.empty())
        return "unknown";
    auto payee = payees.back();
    CScript payeeScript = payee->pdmnState->scriptPayout;

    CTxDestination payeeDest;
    ExtractDestination(payeeScript, payeeDest);

    UniValue obj(UniValue::VOBJ);

    obj.pushKV("height",        (int)(mnList.GetHeight() + heightShift));
    obj.pushKV("IP:port",       payee->pdmnState->addr.ToString());
    obj.pushKV("proTxHash",     payee->proTxHash.ToString());
    obj.pushKV("outpoint",      payee->collateralOutpoint.ToStringShort());
    obj.pushKV("payee",         IsValidDestination(payeeDest) ? EncodeDestination(payeeDest) : "UNKNOWN");
    return obj;
}

static RPCHelpMan masternode_winner()
{
    return RPCHelpMan{"masternode_winner",
        "\nPrint info on next masternode winner to vote for\n",
        {              
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{
                HelpExampleCli("masternode_winner", "")
            + HelpExampleRpc("masternode_winner", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    return GetNextMasternodeForPayment(10);
},
    };
} 

static RPCHelpMan masternode_current()
{
    return RPCHelpMan{"masternode_current",
        "\nPrint info on current masternode winner to be paid the next block (calculated locally)\n",
        {              
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{
                HelpExampleCli("masternode_current", "")
            + HelpExampleRpc("masternode_current", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    return GetNextMasternodeForPayment(1);
},
    };
} 

static RPCHelpMan masternode_status()
{
    return RPCHelpMan{"masternode_status",
        "\nPrint masternode status outputs\n",
        {              
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{
                HelpExampleCli("masternode_status", "")
            + HelpExampleRpc("masternode_status", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    if (!fMasternodeMode)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "This is not a masternode");

    UniValue mnObj(UniValue::VOBJ);

    // keep compatibility with legacy status for now (might get deprecated/removed later)
    mnObj.pushKV("outpoint", activeMasternodeInfo.outpoint.ToStringShort());
    mnObj.pushKV("service", activeMasternodeInfo.service.ToString());
    CDeterministicMNList mnList;
    if(deterministicMNManager)
        deterministicMNManager->GetListAtChainTip(mnList);
    auto dmn = mnList.GetMN(activeMasternodeInfo.proTxHash);
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
},
    };
} 

static RPCHelpMan masternode_winners()
{
    return RPCHelpMan{"masternode_winners",
        "\nPrint list of masternode winners\n",
        {         
            {"count", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "Number of last winners to return."},    
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{
                HelpExampleCli("masternode_winners", "")
            + HelpExampleRpc("masternode_winners", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    int nHeight;
    {
        LOCK(cs_main);
        CBlockIndex* pindex = ::ChainActive().Tip();
        if (!pindex) return NullUniValue;

        nHeight = pindex->nHeight;
    }

    int nLast = 10;
    if (!request.params[0].isNull()) {
        nLast = request.params[0].get_int();
    }


    UniValue obj(UniValue::VOBJ);
    auto mapPayments = GetRequiredPaymentsStrings(nHeight - nLast, nHeight + 20);
    for (const auto &p : mapPayments) {
        obj.pushKV(strprintf("%d", p.first), p.second);
    }

    return obj;
},
    };
} 

RPCHelpMan masternodelist()
{
    return RPCHelpMan{"masternodelist",
        "\nPrint list of masternode list\n",
        {         
            {"mode", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Mode."},
            {"filter", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "filter."},    
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{
                HelpExampleCli("masternodelist", "")
            + HelpExampleRpc("masternodelist", "")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::string strMode = "json";
    std::string strFilter = "";

    if (!request.params[0].isNull()) strMode = request.params[0].get_str();
    if (!request.params[1].isNull()) strFilter = request.params[1].get_str();
    strMode = ToLower(strMode);

    if (strMode != "addr" && strMode != "full" && strMode != "info" && strMode != "json" &&
                strMode != "owneraddress" && strMode != "votingaddress" &&
                strMode != "lastpaidtime" && strMode != "lastpaidblock" &&
                strMode != "payee" && strMode != "pubkeyoperator" &&
                strMode != "status")
    {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid mode");
    }

    UniValue obj(UniValue::VOBJ);
    CDeterministicMNList mnList;
    if(deterministicMNManager)
        deterministicMNManager->GetListAtChainTip(mnList);
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
        const CBlockIndex* pindex = ::ChainActive()[dmn->pdmnState->nLastPaidHeight];
        return (int)pindex->nTime;
    };

    mnList.ForEachMN(false, [&](const CDeterministicMNCPtr& dmn) {
        std::string strOutpoint = dmn->collateralOutpoint.ToStringShort();
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
            if (strFilter !="" && strAddress.find(strFilter) == std::string::npos &&
                strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, strAddress);
        } else if (strMode == "full") {
            std::ostringstream streamFull;
            streamFull << std::setw(18) <<
                           dmnToStatus(dmn) << " " <<
                           payeeStr << " " << std::setw(10) <<
                           dmnToLastPaidTime(dmn) << " "  << std::setw(6) <<
                           dmn->pdmnState->nLastPaidHeight << " " <<
                           dmn->pdmnState->addr.ToString();
            std::string strFull = streamFull.str();
            if (strFilter !="" && strFull.find(strFilter) == std::string::npos &&
                strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, strFull);
        } else if (strMode == "info") {
            std::ostringstream streamInfo;
            streamInfo << std::setw(18) <<
                           dmnToStatus(dmn) << " " <<
                           payeeStr << " " <<
                           dmn->pdmnState->addr.ToString();
            std::string strInfo = streamInfo.str();
            if (strFilter !="" && strInfo.find(strFilter) == std::string::npos &&
                strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, strInfo);
        } else if (strMode == "json") {
            std::ostringstream streamInfo;
            streamInfo <<  dmn->proTxHash.ToString() << " " <<
                           dmn->pdmnState->addr.ToString() << " " <<
                           payeeStr << " " <<
                           dmnToStatus(dmn) << " " <<
                           dmnToLastPaidTime(dmn) << " " <<
                           dmn->pdmnState->nLastPaidHeight << " " <<
                           EncodeDestination(WitnessV0KeyHash(dmn->pdmnState->keyIDOwner)) << " " <<
                           EncodeDestination(WitnessV0KeyHash(dmn->pdmnState->keyIDVoting)) << " " <<
                           collateralAddressStr << " " <<
                           dmn->pdmnState->pubKeyOperator.Get().ToString();
            std::string strInfo = streamInfo.str();
            if (strFilter !="" && strInfo.find(strFilter) == std::string::npos &&
                strOutpoint.find(strFilter) == std::string::npos) return;
            UniValue objMN(UniValue::VOBJ);
            objMN.pushKV("proTxHash", dmn->proTxHash.ToString());
            objMN.pushKV("address", dmn->pdmnState->addr.ToString());
            objMN.pushKV("payee", payeeStr);
            objMN.pushKV("status", dmnToStatus(dmn));
            objMN.pushKV("collateralblock", dmn->pdmnState->nCollateralHeight);
            objMN.pushKV("lastpaidtime", dmnToLastPaidTime(dmn));
            objMN.pushKV("lastpaidblock", dmn->pdmnState->nLastPaidHeight);
            objMN.pushKV("owneraddress", EncodeDestination(WitnessV0KeyHash(dmn->pdmnState->keyIDOwner)));
            objMN.pushKV("votingaddress", EncodeDestination(WitnessV0KeyHash(dmn->pdmnState->keyIDVoting)));
            objMN.pushKV("collateraladdress", collateralAddressStr);
            objMN.pushKV("pubkeyoperator", dmn->pdmnState->pubKeyOperator.Get().ToString());
            obj.pushKV(strOutpoint, objMN);
        } else if (strMode == "lastpaidblock") {
            if (strFilter !="" && strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, dmn->pdmnState->nLastPaidHeight);
        } else if (strMode == "lastpaidtime") {
            if (strFilter !="" && strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, dmnToLastPaidTime(dmn));
        } else if (strMode == "payee") {
            if (strFilter !="" && payeeStr.find(strFilter) == std::string::npos &&
                strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, payeeStr);
        } else if (strMode == "owneraddress") {
            if (strFilter !="" && strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, EncodeDestination(WitnessV0KeyHash(dmn->pdmnState->keyIDOwner)));
        } else if (strMode == "pubkeyoperator") {
            if (strFilter !="" && strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, dmn->pdmnState->pubKeyOperator.Get().ToString());
        } else if (strMode == "status") {
            std::string strStatus = dmnToStatus(dmn);
            if (strFilter !="" && strStatus.find(strFilter) == std::string::npos &&
                strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, strStatus);
        } else if (strMode == "votingaddress") {
            if (strFilter !="" && strOutpoint.find(strFilter) == std::string::npos) return;
            obj.pushKV(strOutpoint, EncodeDestination(WitnessV0KeyHash(dmn->pdmnState->keyIDVoting)));
        }
    });

    return obj;
},
    };
} 

void RegisterMasternodeRPCCommands(CRPCTable &t)
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         argNames
//  --------------------- ------------------------  -----------------------  ----------
    { "masternode",               "masternode_connect",         &masternode_connect,      {"address"} },
    { "masternode",               "masternode_list",            &masternode_list,         {"mode","filter"} },
    { "masternode",               "masternode_winners",         &masternode_winners,      {"count"} },
    { "masternode",               "masternode_count",           &masternode_count,        {} },
    { "masternode",               "masternode_winner",          &masternode_winner,       {} },
    { "masternode",               "masternode_status",          &masternode_status,       {} },
    { "masternode",               "masternode_current",         &masternode_current,      {} },
};
// clang-format on
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
