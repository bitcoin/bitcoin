// Copyright (c) 2014-2017 The Dash Core developers
// Copyright (c) 2017-2018 The Syscoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <activemasternode.h>
#include <base58.h>
#include <clientversion.h>
#include <init.h>
#include <netbase.h>
#include <validation.h>
#include <masternodepayments.h>
#include <masternodesync.h>
#include <masternodeconfig.h>
#include <masternodeman.h>
#include <rpc/server.h>
#include <util/moneystr.h>
#include <key_io.h>
#include <fstream>
#include <iomanip>
#include <univalue.h>
#include <wallet/wallet.h>
#include <wallet/rpcwallet.h>
#include <rpc/util.h>
UniValue masternodelist(const JSONRPCRequest& request);

bool EnsureWalletIsAvailable(CWallet * const pwallet, bool avoidException);
#ifdef ENABLE_WALLET
void EnsureWalletIsUnlocked(CWallet * const pwallet);
#endif


UniValue masternode(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> wallet = GetWalletForJSONRPCRequest(request);
    CWallet* pwallet = wallet.get();
    std::string strCommand;
    if (request.params.size() >= 1) {
        strCommand = request.params[0].get_str();
    }

#ifdef ENABLE_WALLET
    if (strCommand == "start-many")
        throw JSONRPCError(RPC_INVALID_PARAMETER, "DEPRECATED, please use start-all instead");
#endif // ENABLE_WALLET

    if (request.fHelp  ||
        (
#ifdef ENABLE_WALLET
            strCommand != "initialize" && strCommand != "start-all" && strCommand != "start-missing" &&
         strCommand != "start-disabled" && strCommand != "outputs" &&
#endif // ENABLE_WALLET
         strCommand != "list" && strCommand != "list-conf" && strCommand != "count" &&
         strCommand != "debug" && strCommand != "current" && strCommand != "winner" && strCommand != "winners" && strCommand != "genkey" &&
         strCommand != "connect" && strCommand != "status"))
            throw std::runtime_error(
                RPCHelpMan{"masternode",
                "\nSet of commands to execute masternode related actions\n"
                "\nArguments:\n"
                "1. \"command\"        (string or set of strings, required) The command to execute\n"
                "\nAvailable commands:\n"
                "  count        - Get information about number of masternodes (DEPRECATED options: 'total', 'enabled', 'qualify', 'all')\n"
                "  current      - Print info on current masternode winner to be paid the next block (calculated locally)\n"
                "  genkey       - Generate new masternodeprivkey\n"
#ifdef ENABLE_WALLET
                "  outputs      - Print masternode compatible outputs\n"
                "  initialize  - Start single remote masternode by assigned name configured in masternode.conf\n"
                "  start-<mode> - Start remote masternodes configured in masternode.conf (<mode>: 'all', 'missing', 'disabled')\n"
#endif // ENABLE_WALLET
                "  status       - Print masternode status information\n"
                "  list         - Print list of all known masternodes (see masternodelist for more info)\n"
                "  list-conf    - Print masternode.conf in JSON format\n"
                "  winner       - Print info on next masternode winner to vote for\n"
                "  winners      - Print list of masternode winners\n",
                    {
                        {"command", RPCArg::Type::STR, RPCArg::Optional::NO, "(count|current|genkey|outputs|initialize|start-<mode>|status|list|list-conf|winner|winners"}
                    },
                    RPCResult{""},
                    RPCExamples{
                        HelpExampleCli("masternode", "list")
                        + HelpExampleRpc("masternode", "list")
                    },
                }.ToString());

    if (strCommand == "list")
    {
        JSONRPCRequest newRequest = request;
        newRequest.params.setArray();
        // forward params but skip "list"
        for (unsigned int i = 1; i < request.params.size(); i++) {
            newRequest.params.push_back(request.params[i]);
        }
        return masternodelist(newRequest);
    }

    if(strCommand == "connect")
    {
        if (request.params.size() < 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Masternode address required");

        std::string strAddress = request.params[1].get_str();

        CService addr;
        if (!Lookup(strAddress.c_str(), addr, 0, false))
            throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("Incorrect masternode address %s", strAddress));

        // TODO: Pass CConnman instance somehow and don't use global variable.
        g_connman->OpenMasternodeConnection(CAddress(addr, NODE_NETWORK));
        if (!g_connman->IsConnected(CAddress(addr, NODE_NETWORK), CConnman::AllNodes))
            throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("Couldn't connect to masternode %s", strAddress));

        return "successfully connected";
    }

    if (strCommand == "count")
    {
        if (request.params.size() > 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Too many parameters");

        int nCount;
        masternode_info_t mnInfo;
        mnodeman.GetNextMasternodeInQueueForPayment(true, nCount, mnInfo);

        int total = mnodeman.size();
        int enabled = mnodeman.CountEnabled();

        if (request.params.size() == 1) {
            UniValue obj(UniValue::VOBJ);

            obj.pushKV("total", total);
            obj.pushKV("enabled", enabled);
            obj.pushKV("qualify", nCount);

            return obj;
        }

        std::string strMode = request.params[1].get_str();

        if (strMode == "total")
            return total;
         

        if (strMode == "enabled")
            return enabled;

        if (strMode == "qualify")
            return nCount;

        if (strMode == "all")
            return strprintf("Total: %d (Enabled: %d / Qualify: %d)",
                total, enabled, nCount);
    }

    if (strCommand == "current" || strCommand == "winner")
    {
        int nCount;
        int nHeight;
        masternode_info_t mnInfo;
        {
            LOCK(cs_main);
            CBlockIndex* pindex = chainActive.Tip();
            mnodeman.UpdateLastPaid(pindex);
            nHeight = pindex->nHeight + (strCommand == "current" ? 1 : 10);
        }
        
        if(!mnodeman.GetNextMasternodeInQueueForPayment(nHeight, true, nCount, mnInfo))
            return "unknown";

        UniValue obj(UniValue::VOBJ);

        obj.pushKV("height",        nHeight);
        obj.pushKV("IP:port",       mnInfo.addr.ToString());
        obj.pushKV("protocol",      mnInfo.nProtocolVersion);
        obj.pushKV("outpoint",      mnInfo.outpoint.ToStringShort());
        obj.pushKV("payee",         EncodeDestination(mnInfo.pubKeyCollateralAddress.GetID()));
        obj.pushKV("lastseen",      mnInfo.nTimeLastPing);
        int64_t activeTime = (int64_t)(mnInfo.nTimeLastPing - mnInfo.sigTime);
        if(activeTime <= 0)
            activeTime = (int64_t)(GetAdjustedTime() - mnInfo.sigTime);
        obj.pushKV("activeseconds", activeTime);
        return obj;
    }

#ifdef ENABLE_WALLET
    if (strCommand == "initialize")
    {
        if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
            return NullUniValue;

        if (request.params.size() < 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Please specify a name");

        {
            LOCK(pwallet->cs_wallet);
            EnsureWalletIsUnlocked(pwallet);
        }

        std::string strAlias = request.params[1].get_str();

        bool fFound = false;

        UniValue statusObj(UniValue::VOBJ);
        statusObj.pushKV("name", strAlias);

        for (const auto& mne : masternodeConfig.getEntries()) {
            if(mne.getAlias() == strAlias) {
                fFound = true;
                std::string strError;
                CMasternodeBroadcast mnb;
       
                bool fResult = CMasternodeBroadcast::Create(pwallet, mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strError, mnb);

                int nDoS;
                if (fResult && !mnodeman.CheckMnbAndUpdateMasternodeList(NULL, mnb, nDoS, *g_connman)) {
                    strError = "Failed to verify MNB";
                    fResult = false;
                }

                statusObj.pushKV("result", fResult ? "successful" : "failed");
                if(!fResult) {
                    statusObj.pushKV("errorMessage", strError);
                }
                mnodeman.NotifyMasternodeUpdates(*g_connman);
                break;
            }
        }

        if(!fFound) {
            statusObj.pushKV("result", "failed");
            statusObj.pushKV("errorMessage", "Could not find name in config. Verify with list-conf.");
        }

        return statusObj;

    }

    if (strCommand == "start-all" || strCommand == "start-missing" || strCommand == "start-disabled")
    {
        if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
            return NullUniValue;

        {
            LOCK(pwallet->cs_wallet);
            EnsureWalletIsUnlocked(pwallet);
        }

        if((strCommand == "start-missing" || strCommand == "start-disabled") && !masternodeSync.IsMasternodeListSynced()) {
            throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "You can't use this command until masternode list is synced");
        }

        int nSuccessful = 0;
        int nFailed = 0;

        UniValue resultsObj(UniValue::VOBJ);

        for (const auto& mne : masternodeConfig.getEntries()) {
            std::string strError;
            uint32_t outputIndex;
            bool fResult;
            if(!ParseUInt32(mne.getOutputIndex(), &outputIndex)){
                strError = "Could not parse output index";
                fResult = false;
            }
            COutPoint outpoint = COutPoint(uint256S(mne.getTxHash()), outputIndex);
            CMasternode mn;
            bool fFound = mnodeman.Get(outpoint, mn);
            CMasternodeBroadcast mnb;

            if(strCommand == "start-missing" && fFound) continue;
            if(strCommand == "start-disabled" && fFound && mn.IsEnabled()) continue;
            fResult = fResult && CMasternodeBroadcast::Create(pwallet, mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strError, mnb);

            int nDoS;
            if (fResult && !mnodeman.CheckMnbAndUpdateMasternodeList(NULL, mnb, nDoS, *g_connman)) {
                strError = "Failed to verify MNB";
                fResult = false;
            }

            UniValue statusObj(UniValue::VOBJ);
            statusObj.pushKV("name", mne.getAlias());
            statusObj.pushKV("result", fResult ? "successful" : "failed");

            if (fResult) {
                nSuccessful++;
            } else {
                nFailed++;
                statusObj.pushKV("errorMessage", strError);
            }

            resultsObj.pushKV("status", statusObj);
        }
        mnodeman.NotifyMasternodeUpdates(*g_connman);

        UniValue returnObj(UniValue::VOBJ);
        returnObj.pushKV("overall", strprintf("Successfully started %d masternodes, failed to start %d, total %d", nSuccessful, nFailed, nSuccessful + nFailed));
        returnObj.pushKV("detail", resultsObj);

        return returnObj;
    }
#endif // ENABLE_WALLET

    if (strCommand == "genkey")
    {
        CKey secret;
        secret.MakeNewKey(false);

        return EncodeSecret(secret);
    }

    if (strCommand == "list-conf")
    {
        UniValue resultObj(UniValue::VOBJ);

        for (const auto& mne : masternodeConfig.getEntries()) {
            uint32_t outputIndex;
            if(!ParseUInt32(mne.getOutputIndex(), &outputIndex)){
                continue;
            }
            COutPoint outpoint = COutPoint(uint256S(mne.getTxHash()), outputIndex);
            CMasternode mn;
            bool fFound = mnodeman.Get(outpoint, mn);

            std::string strStatus = fFound ? mn.GetStatus() : "MISSING";

            UniValue mnObj(UniValue::VOBJ);
            mnObj.pushKV("name", mne.getAlias());
            mnObj.pushKV("address", mne.getIp());
            mnObj.pushKV("privateKey", mne.getPrivKey());
            mnObj.pushKV("txHash", mne.getTxHash());
            mnObj.pushKV("outputIndex", mne.getOutputIndex());
            mnObj.pushKV("status", strStatus);
            resultObj.pushKV("masternode", mnObj);
        }

        return resultObj;
    }

#ifdef ENABLE_WALLET
    if (strCommand == "outputs") {
        if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
            return NullUniValue;
        LOCK(pwallet->cs_wallet);
        // Find possible candidates
        std::vector<COutput> vPossibleCoins;
        auto locked_chain = pwallet->chain().lock();
        pwallet->AvailableCoins(*locked_chain, vPossibleCoins, true, nullptr, 1, MAX_MONEY, MAX_MONEY, 0, 0, 9999999);
   
        UniValue obj(UniValue::VOBJ);
        for (const auto& out : vPossibleCoins) {
            obj.pushKV(out.tx->GetHash().ToString(), strprintf("%d", out.i));
        }

        return obj;
    }
#endif // ENABLE_WALLET

    if (strCommand == "status")
    {
        if (!fMasternodeMode)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "This is not a masternode");

        UniValue mnObj(UniValue::VOBJ);

        mnObj.pushKV("outpoint", activeMasternode.outpoint.ToStringShort());
        mnObj.pushKV("service", activeMasternode.service.ToString());

        CMasternode mn;
        if(mnodeman.Get(activeMasternode.outpoint, mn)) {
            mnObj.pushKV("payee", EncodeDestination(mn.pubKeyCollateralAddress.GetID()));
        }

        mnObj.pushKV("status", activeMasternode.GetStatus());
        return mnObj;
    }

    if (strCommand == "winners")
    {
        int nHeight;
        {
            LOCK(cs_main);
            CBlockIndex* pindex = chainActive.Tip();
            if(!pindex) return NullUniValue;

            nHeight = pindex->nHeight;
        }

        int nLast = 10;
        std::string strFilter = "";

        if (request.params.size() >= 2) {
            if(!ParseInt32(request.params[1].get_str(), &nLast))
                nLast = 10;
        }

        if (request.params.size() == 3) {
            strFilter = request.params[2].get_str();
        }

        if (request.params.size() > 3)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'masternode winners ( \"count\" \"filter\" )'");

        UniValue obj(UniValue::VOBJ);

        for(int i = nHeight - nLast; i < nHeight + 20; i++) {
            std::string strPayment = GetRequiredPaymentsString(i);
            if (strFilter !="" && strPayment.find(strFilter) == std::string::npos) continue;
            obj.pushKV(strprintf("%d", i), strPayment);
        }

        return obj;
    }

    return NullUniValue;
}

UniValue masternodelist(const JSONRPCRequest& request)
{
    std::string strMode = "json";
    std::string strFilter = "";

    if (request.params.size() >= 1) strMode = request.params[0].get_str();
    if (request.params.size() == 2) strFilter = request.params[1].get_str();

    if (request.fHelp || (
                strMode != "activeseconds" && strMode != "addr" && strMode != "daemon" && strMode != "full" && strMode != "info" && strMode != "json" &&
                strMode != "lastseen" && strMode != "lastpaidtime" && strMode != "lastpaidblock" &&
                strMode != "protocol" && strMode != "payee" && strMode != "pubkey" &&
                strMode != "rank" && strMode != "sentinel" && strMode != "status"))
    {
        throw std::runtime_error(
                "masternodelist ( \"mode\" \"filter\" )\n"
                "Get a list of masternodes in different modes\n"
                "\nArguments:\n"
                "1. \"mode\"      (string, optional/required to use filter, defaults = json) The mode to run list in\n"
                "2. \"filter\"    (string, optional) Filter results. Partial match by outpoint by default in all modes,\n"
                "                                    additional matches in some modes are also available\n"
                "\nAvailable modes:\n"
                "  activeseconds  - Print number of seconds masternode recognized by the network as enabled\n"
                "                   (since latest issued \"masternode start/start-many/initialize\")\n"
                "  addr           - Print ip address associated with a masternode (can be additionally filtered, partial match)\n"
                "  daemon         - Print daemon version of a masternode (can be additionally filtered, exact match)\n"
                "  full           - Print info in format 'status protocol payee lastseen activeseconds lastpaidtime lastpaidblock IP pingretries'\n"
                "                   (can be additionally filtered, partial match)\n"
                "  info           - Print info in format 'status protocol payee lastseen activeseconds sentinelversion sentinelstate IP pingretries'\n"
                "                   (can be additionally filtered, partial match)\n"
                "  json           - Print info in JSON format (can be additionally filtered, partial match)\n"
                "  lastpaidblock  - Print the last block height a node was paid on the network\n"
                "  lastpaidtime   - Print the last time a node was paid on the network\n"
                "  lastseen       - Print timestamp of when a masternode was last seen on the network\n"
                "  payee          - Print Syscoin address associated with a masternode (can be additionally filtered,\n"
                "                   partial match)\n"
                "  protocol       - Print protocol of a masternode (can be additionally filtered, exact match)\n"
                "  pubkey         - Print the masternode (not collateral) public key\n"
                "  rank           - Print rank of a masternode based on current block\n"
                "  sentinel       - Print sentinel version of a masternode (can be additionally filtered, exact match)\n"
                "  status         - Print masternode status: PRE_ENABLED / ENABLED / NEW_START_REQUIRED /\n"
                "                   UPDATE_REQUIRED / POSE_BAN / OUTPOINT_SPENT (can be additionally filtered, partial match)\n"
                );
    }
    if (strMode == "full" || strMode == "json" || strMode == "lastpaidtime" || strMode == "lastpaidblock") {
        {
            LOCK(cs_main);
            CBlockIndex* pindex = chainActive.Tip();
            mnodeman.UpdateLastPaid(pindex);
        }
    }

    UniValue obj(UniValue::VOBJ);
    if (strMode == "rank") {
        CMasternodeMan::rank_pair_vec_t vMasternodeRanks;
        mnodeman.GetMasternodeRanks(vMasternodeRanks);
        for (const auto& rankpair : vMasternodeRanks) {
            std::string strOutpoint = rankpair.second.outpoint.ToStringShort();
            if (strFilter !="" && strOutpoint.find(strFilter) == std::string::npos) continue;
            obj.pushKV(strOutpoint, rankpair.first);
        }
    } else {
        const std::map<COutPoint, CMasternode> &mapMasternodes = mnodeman.GetFullMasternodeMap();
        for (const auto& mnpair : mapMasternodes) {
            CMasternode mn = mnpair.second;
            std::string strOutpoint = mnpair.first.ToStringShort();
            if (strMode == "activeseconds") {
                if (strFilter !="" && strOutpoint.find(strFilter) == std::string::npos) continue;
                obj.pushKV(strOutpoint, (int64_t)(mn.lastPing.sigTime - mn.sigTime));
            } else if (strMode == "addr") {
                std::string strAddress = mn.addr.ToString();
                if (strFilter !="" && strAddress.find(strFilter) == std::string::npos &&
                    strOutpoint.find(strFilter) == std::string::npos) continue;
                obj.pushKV(strOutpoint, strAddress);
            } else if (strMode == "daemon") {
                std::string strDaemon = mn.lastPing.GetDaemonString();
                if (strFilter !="" && strDaemon.find(strFilter) == std::string::npos &&
                    strOutpoint.find(strFilter) == std::string::npos) continue;
                obj.pushKV(strOutpoint, strDaemon);
            } else if (strMode == "sentinel") {
                std::string strSentinel = mn.lastPing.GetSentinelString();
                if (strFilter !="" && strSentinel.find(strFilter) == std::string::npos &&
                    strOutpoint.find(strFilter) == std::string::npos) continue;
                obj.pushKV(strOutpoint, strSentinel);
            } else if (strMode == "full") {
                std::ostringstream streamFull;
                streamFull << std::setw(18) <<
                               mn.GetStatus() << " " <<
                               mn.nProtocolVersion << " " <<
                               EncodeDestination(mn.pubKeyCollateralAddress.GetID()) << " " <<
                               (int64_t)mn.lastPing.sigTime << " " << std::setw(8) <<
                               (int64_t)(mn.lastPing.sigTime - mn.sigTime) << " " << std::setw(10) <<
                               mn.GetLastPaidTime() << " "  << std::setw(6) <<
                               mn.GetLastPaidBlock() << " " <<
                               mn.addr.ToString() << " " <<
                               mn.nPingRetries;
                std::string strFull = streamFull.str();
                if (strFilter !="" && strFull.find(strFilter) == std::string::npos &&
                    strOutpoint.find(strFilter) == std::string::npos) continue;
                obj.pushKV(strOutpoint, strFull);
            } else if (strMode == "info") {
                std::ostringstream streamInfo;
                streamInfo << std::setw(18) <<
                               mn.GetStatus() << " " <<
                               mn.nProtocolVersion << " " <<
                               EncodeDestination(mn.pubKeyCollateralAddress.GetID()) << " " <<
                               (int64_t)mn.lastPing.sigTime << " " << std::setw(8) <<
                               (int64_t)(mn.lastPing.sigTime - mn.sigTime) << " " <<
                               mn.lastPing.GetSentinelString() << " "  <<
                               (mn.lastPing.fSentinelIsCurrent ? "current" : "expired") << " " <<
                               mn.addr.ToString() << " " <<
                               mn.nPingRetries;
                std::string strInfo = streamInfo.str();
                if (strFilter !="" && strInfo.find(strFilter) == std::string::npos &&
                    strOutpoint.find(strFilter) == std::string::npos) continue;
                obj.pushKV(strOutpoint, strInfo);
            } else if (strMode == "json") {
                std::ostringstream streamInfo;
                streamInfo <<  mn.addr.ToString() << " " <<
                               EncodeDestination(mn.pubKeyCollateralAddress.GetID()) << " " <<
                               mn.GetStatus() << " " <<
                               mn.nProtocolVersion << " " <<
                               mn.lastPing.nDaemonVersion << " " <<
                               mn.lastPing.GetSentinelString() << " " <<
                               (mn.lastPing.fSentinelIsCurrent ? "current" : "expired") << " " <<
                               (int64_t)mn.lastPing.sigTime << " " <<
                               (int64_t)(mn.lastPing.sigTime - mn.sigTime) << " " <<
                               mn.GetLastPaidTime() << " " <<
                               mn.GetLastPaidBlock() << " " <<
                               mn.nPingRetries;
                std::string strInfo = streamInfo.str();
                if (strFilter !="" && strInfo.find(strFilter) == std::string::npos &&
                    strOutpoint.find(strFilter) == std::string::npos) continue;
                UniValue objMN(UniValue::VOBJ);
                objMN.pushKV("address", mn.addr.ToString());
                objMN.pushKV("payee", EncodeDestination(mn.pubKeyCollateralAddress.GetID()));
                objMN.pushKV("status", mn.GetStatus());
                objMN.pushKV("protocol", mn.nProtocolVersion);
                objMN.pushKV("daemonversion", mn.lastPing.GetDaemonString());
                objMN.pushKV("sentinelversion", mn.lastPing.GetSentinelString());
                objMN.pushKV("sentinelstate", (mn.lastPing.fSentinelIsCurrent ? "current" : "expired"));
                objMN.pushKV("lastseen", (int64_t)mn.lastPing.sigTime);
                int64_t activeTime = (int64_t)(mn.lastPing.sigTime - mn.sigTime);
                if(activeTime <= 0)
                    activeTime = (int64_t)(GetAdjustedTime() - mn.sigTime);
                objMN.pushKV("activeseconds", activeTime);
                objMN.pushKV("lastpaidtime", mn.GetLastPaidTime());
                objMN.pushKV("lastpaidblock", mn.GetLastPaidBlock());
                objMN.pushKV("pingretries", mn.nPingRetries);
                obj.pushKV(strOutpoint, objMN);
            } else if (strMode == "lastpaidblock") {
                if (strFilter !="" && strOutpoint.find(strFilter) == std::string::npos) continue;
                obj.pushKV(strOutpoint, mn.GetLastPaidBlock());
            } else if (strMode == "lastpaidtime") {
                if (strFilter !="" && strOutpoint.find(strFilter) == std::string::npos) continue;
                obj.pushKV(strOutpoint, mn.GetLastPaidTime());
            } else if (strMode == "lastseen") {
                if (strFilter !="" && strOutpoint.find(strFilter) == std::string::npos) continue;
                obj.pushKV(strOutpoint, (int64_t)mn.lastPing.sigTime);
            } else if (strMode == "payee") {
                std::string strPayee = EncodeDestination(mn.pubKeyCollateralAddress.GetID());
                if (strFilter !="" && strPayee.find(strFilter) == std::string::npos &&
                    strOutpoint.find(strFilter) == std::string::npos) continue;
                obj.pushKV(strOutpoint, strPayee);
            } else if (strMode == "protocol") {
                if (strFilter !="" && strFilter != strprintf("%d", mn.nProtocolVersion) &&
                    strOutpoint.find(strFilter) == std::string::npos) continue;
                obj.pushKV(strOutpoint, mn.nProtocolVersion);
            } else if (strMode == "pubkey") {
                if (strFilter !="" && strOutpoint.find(strFilter) == std::string::npos) continue;
                obj.pushKV(strOutpoint, HexStr(mn.pubKeyMasternode));
            } else if (strMode == "status") {
                std::string strStatus = mn.GetStatus();
                if (strFilter !="" && strStatus.find(strFilter) == std::string::npos &&
                    strOutpoint.find(strFilter) == std::string::npos) continue;
                obj.pushKV(strOutpoint, strStatus);
            }
        }
    }
    return obj;
}

bool DecodeHexVecMnb(std::vector<CMasternodeBroadcast>& vecMnb, std::string strHexMnb) {

    if (!IsHex(strHexMnb))
        return false;

    std::vector<unsigned char> mnbData(ParseHex(strHexMnb));
    CDataStream ssData(mnbData, SER_NETWORK, MIN_PEER_PROTO_VERSION);
    try {
        ssData >> vecMnb;
    }
    catch (const std::exception&) {
        return false;
    }

    return true;
}

UniValue masternodebroadcast(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    std::string strCommand;
    if (request.params.size() >= 1)
        strCommand = request.params[0].get_str();

    if (request.fHelp  ||
        (
#ifdef ENABLE_WALLET
            strCommand != "create-name" && strCommand != "create-all" &&
#endif // ENABLE_WALLET
            strCommand != "decode" && strCommand != "relay"))
        throw std::runtime_error(
                "masternodebroadcast \"command\"...\n"
                "Set of commands to create and relay masternode broadcast messages\n"
                "\nArguments:\n"
                "1. \"command\"        (string or set of strings, required) The command to execute\n"
                "\nAvailable commands:\n"
#ifdef ENABLE_WALLET
                "  create-name  - Create single remote masternode broadcast message by assigned name configured in masternode.conf\n"
                "  create-all    - Create remote masternode broadcast messages for all masternodes configured in masternode.conf\n"
#endif // ENABLE_WALLET
                "  decode        - Decode masternode broadcast message\n"
                "  relay         - Relay masternode broadcast message to the network\n"
                );

#ifdef ENABLE_WALLET
    if (strCommand == "create-name")
    {
        if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
            return NullUniValue;

        // wait for reindex and/or import to finish
        if (fImporting || fReindex)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Wait for reindex and/or import to finish");

        if (request.params.size() < 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Please specify a name");

        {
            LOCK(pwallet->cs_wallet);
            EnsureWalletIsUnlocked(pwallet);
        }

        bool fFound = false;
        std::string strAlias = request.params[1].get_str();

        UniValue statusObj(UniValue::VOBJ);
        std::vector<CMasternodeBroadcast> vecMnb;

        statusObj.pushKV("name", strAlias);

        for (const auto& mne : masternodeConfig.getEntries()) {
            if(mne.getAlias() == strAlias) {
                fFound = true;
                std::string strError;
                CMasternodeBroadcast mnb;
                bool fResult = CMasternodeBroadcast::Create(pwallet, mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strError, mnb, true);

                statusObj.pushKV("result", fResult ? "successful" : "failed");
                if(fResult) {
                    vecMnb.push_back(mnb);
                    CDataStream ssVecMnb(SER_NETWORK, MIN_PEER_PROTO_VERSION);
                    ssVecMnb << vecMnb;
                    statusObj.pushKV("hex", HexStr(ssVecMnb));
                } else {
                    statusObj.pushKV("errorMessage", strError);
                }
                break;
            }
        }

        if(!fFound) {
            statusObj.pushKV("result", "not found");
            statusObj.pushKV("errorMessage", "Could not find name in config. Verify with list-conf.");
        }

        return statusObj;

    }

    if (strCommand == "create-all")
    {
        if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
            return NullUniValue;

        // wait for reindex and/or import to finish
        if (fImporting || fReindex)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Wait for reindex and/or import to finish");

        {
            LOCK(pwallet->cs_wallet);
            EnsureWalletIsUnlocked(pwallet);
        }

        int nSuccessful = 0;
        int nFailed = 0;

        UniValue resultsObj(UniValue::VOBJ);
        std::vector<CMasternodeBroadcast> vecMnb;

        for (const auto& mne : masternodeConfig.getEntries()) {
            std::string strError;
            CMasternodeBroadcast mnb;
            bool fResult = CMasternodeBroadcast::Create(pwallet, mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strError, mnb, true);

            UniValue statusObj(UniValue::VOBJ);
            statusObj.pushKV("name", mne.getAlias());
            statusObj.pushKV("result", fResult ? "successful" : "failed");

            if(fResult) {
                nSuccessful++;
                vecMnb.push_back(mnb);
            } else {
                nFailed++;
                statusObj.pushKV("errorMessage", strError);
            }

            resultsObj.pushKV("status", statusObj);
        }

        CDataStream ssVecMnb(SER_NETWORK, MIN_PEER_PROTO_VERSION);
        ssVecMnb << vecMnb;
        UniValue returnObj(UniValue::VOBJ);
        returnObj.pushKV("overall", strprintf("Successfully created broadcast messages for %d masternodes, failed to create %d, total %d", nSuccessful, nFailed, nSuccessful + nFailed));
        returnObj.pushKV("detail", resultsObj);
        returnObj.pushKV("hex", HexStr(ssVecMnb.begin(), ssVecMnb.end()));

        return returnObj;
    }
#endif // ENABLE_WALLET

    if (strCommand == "decode")
    {
        if (request.params.size() != 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'masternodebroadcast decode \"hexstring\"'");

        std::vector<CMasternodeBroadcast> vecMnb;

        if (!DecodeHexVecMnb(vecMnb, request.params[1].get_str()))
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Masternode broadcast message decode failed");

        int nSuccessful = 0;
        int nFailed = 0;
        int nDos = 0;
        UniValue returnObj(UniValue::VOBJ);

        for (const auto& mnb : vecMnb) {
            UniValue resultObj(UniValue::VOBJ);

            if(mnb.CheckSignature(nDos)) {
                nSuccessful++;
                resultObj.pushKV("outpoint", mnb.outpoint.ToStringShort());
                resultObj.pushKV("addr", mnb.addr.ToString());
                resultObj.pushKV("pubKeyCollateralAddress", EncodeDestination(mnb.pubKeyCollateralAddress.GetID()));
                resultObj.pushKV("pubKeyMasternode", EncodeDestination(mnb.pubKeyMasternode.GetID()));
                resultObj.pushKV("vchSig", EncodeBase64(&mnb.vchSig[0], mnb.vchSig.size()));
                resultObj.pushKV("sigTime", mnb.sigTime);
                resultObj.pushKV("protocolVersion", mnb.nProtocolVersion);

                UniValue lastPingObj(UniValue::VOBJ);
                lastPingObj.pushKV("outpoint", mnb.lastPing.masternodeOutpoint.ToStringShort());
                lastPingObj.pushKV("blockHash", mnb.lastPing.blockHash.ToString());
                lastPingObj.pushKV("sigTime", mnb.lastPing.sigTime);
                lastPingObj.pushKV("vchSig", EncodeBase64(&mnb.lastPing.vchSig[0], mnb.lastPing.vchSig.size()));

                resultObj.pushKV("lastPing", lastPingObj);
            } else {
                nFailed++;
                resultObj.pushKV("errorMessage", "Masternode broadcast signature verification failed");
            }

            returnObj.pushKV(mnb.GetHash().ToString(), resultObj);
        }

        returnObj.pushKV("overall", strprintf("Successfully decoded broadcast messages for %d masternodes, failed to decode %d, total %d", nSuccessful, nFailed, nSuccessful + nFailed));

        return returnObj;
    }

    if (strCommand == "relay")
    {
        if (request.params.size() < 2 || request.params.size() > 3)
            throw JSONRPCError(RPC_INVALID_PARAMETER,   "masternodebroadcast relay \"hexstring\"\n"
                                                        "\nArguments:\n"
                                                        "1. \"hex\"      (string, required) Broadcast messages hex string\n");

        std::vector<CMasternodeBroadcast> vecMnb;

        if (!DecodeHexVecMnb(vecMnb, request.params[1].get_str()))
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Masternode broadcast message decode failed");

        int nSuccessful = 0;
        int nFailed = 0;
        UniValue returnObj(UniValue::VOBJ);

        // verify all signatures first, bailout if any of them broken
        for (const auto& mnb : vecMnb) {
            UniValue resultObj(UniValue::VOBJ);

            resultObj.pushKV("outpoint", mnb.outpoint.ToStringShort());
            resultObj.pushKV("addr", mnb.addr.ToString());

            int nDos = 0;
            bool fResult;
            if (mnb.CheckSignature(nDos)) {
                fResult = mnodeman.CheckMnbAndUpdateMasternodeList(NULL, mnb, nDos, *g_connman);
                mnodeman.NotifyMasternodeUpdates(*g_connman);
            } else fResult = false;

            if(fResult) {
                nSuccessful++;
                resultObj.pushKV(mnb.GetHash().ToString(), "successful");
            } else {
                nFailed++;
                resultObj.pushKV("errorMessage", "Masternode broadcast signature verification failed");
            }

            returnObj.pushKV(mnb.GetHash().ToString(), resultObj);
        }

        returnObj.pushKV("overall", strprintf("Successfully relayed broadcast messages for %d masternodes, failed to relay %d, total %d", nSuccessful, nFailed, nSuccessful + nFailed));

        return returnObj;
    }

    return NullUniValue;
}

UniValue sentinelping(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            "sentinelping version\n"
            "\nSentinel ping.\n"
            "\nArguments:\n"
            "1. version           (numeric, required) Sentinel version\n"
            "\nResult:\n"
            "state                (boolean) Ping result\n"
            "\nExamples:\n"
            + HelpExampleCli("sentinelping", "1000000")
            + HelpExampleRpc("sentinelping", "1000001")
        );
    }

    activeMasternode.UpdateSentinelPing(request.params[0].get_int());
    return true;
}
