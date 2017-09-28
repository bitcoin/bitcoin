// Copyright (c) 2014-2017 The Dash Core developers
// Copyright (c) 2018 PM-Tech
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <activemasternode.h>
#include <base58.h>
#include <init.h>
#include <net.h>
#include <netbase.h>
#include <validation.h>
#include <masternode-payments.h>
#include <masternode-sync.h>
#include <masternodeconfig.h>
#include <masternodeman.h>
#ifdef ENABLE_WALLET
#include <wallet/privatesend-client.h>
#include <wallet/rpcwallet.h>
#endif // ENABLE_WALLET
#include <privatesend-server.h>
#include <rpc/server.h>
#include <util/system.h>
#include <util/moneystr.h>

#include <fstream>
#include <iomanip>
#include <univalue.h>

UniValue masternodelist(const JSONRPCRequest& request);


#ifdef ENABLE_WALLET
void EnsureWalletIsUnlocked(CWallet* const pwallet);

UniValue privatesend(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "privatesend \"command\"\n"
            "\nArguments:\n"
            "1. \"command\"        (string or set of strings, required) The command to execute\n"
            "\nAvailable commands:\n"
            "  start       - Start mixing\n"
            "  stop        - Stop mixing\n"
            "  reset       - Reset mixing\n"
            );

    if(fMasternodeMode)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Client-side mixing is not supported on masternodes");

    if(request.params[0].get_str() == "start") {
        {
            LOCK2(cs_main, pwallet ? &pwallet->cs_wallet : nullptr);
            if (pwallet->IsLocked(true))
                throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please unlock wallet for mixing with walletpassphrase first.");
        }

        bool result = privateSendClient.DoAutomaticDenominating();
        return "Mixing " + (result ? "started successfully" : ("start failed: " + privateSendClient.GetStatus() + ", will retry"));
    }

    if(request.params[0].get_str() == "stop") {
        privateSendClient.fEnablePrivateSend = false;
        return "Mixing was stopped";
    }

    if(request.params[0].get_str() == "reset") {
        privateSendClient.ResetPool();
        return "Mixing was reset";
    }

    return "Unknown command, please see \"help privatesend\"";
}
#endif // ENABLE_WALLET

UniValue getpoolinfo(const JSONRPCRequest& request)
{    
    if (request.fHelp || request.params.size() != 0)
        throw std::runtime_error(
            "getpoolinfo\n"
            "Returns an object containing mixing pool related information.\n");

#ifdef ENABLE_WALLET
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    CPrivateSendBase* pprivateSendBase = fMasternodeMode ? (CPrivateSendBase*)&privateSendServer : (CPrivateSendBase*)&privateSendClient;

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("state",             pprivateSendBase->GetStateString());
    obj.pushKV("mixing_mode",       (!fMasternodeMode && privateSendClient.fPrivateSendMultiSession) ? "multi-session" : "normal");
    obj.pushKV("queue",             pprivateSendBase->GetQueueSize());
    obj.pushKV("entries",           pprivateSendBase->GetEntriesCount());
    obj.pushKV("status",            privateSendClient.GetStatus());

    masternode_info_t mnInfo;
    if (privateSendClient.GetMixingMasternodeInfo(mnInfo)) {
        obj.pushKV("outpoint",      mnInfo.outpoint.ToStringShort());
        obj.pushKV("addr",          mnInfo.addr.ToString());
    }

    if (pwallet) {
        obj.pushKV("keys_left",     pwallet->nKeysLeftSinceAutoBackup);
        obj.pushKV("warnings",      pwallet->nKeysLeftSinceAutoBackup < PRIVATESEND_KEYS_THRESHOLD_WARNING
                                                ? "WARNING: keypool is almost depleted!" : "");
    }
#else // ENABLE_WALLET
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("state",             privateSendServer.GetStateString());
    obj.pushKV("queue",             privateSendServer.GetQueueSize());
    obj.pushKV("entries",           privateSendServer.GetEntriesCount());
#endif // ENABLE_WALLET
    return obj;
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

    if (request.fHelp  ||
        (
#ifdef ENABLE_WALLET
            strCommand != "start-alias" && strCommand != "start-all" && strCommand != "start-missing" &&
         strCommand != "start-disabled" && strCommand != "outputs" &&
#endif // ENABLE_WALLET
         strCommand != "list" && strCommand != "list-conf" && strCommand != "count" && strCommand != "debug" && strCommand != "current" &&
         strCommand != "winner" && strCommand != "winners" && strCommand != "genkey" && strCommand != "status"))
            throw std::runtime_error(
                "masternode \"command\"...\n"
                "Set of commands to execute masternode related actions\n"
                "\nArguments:\n"
                "1. \"command\"        (string or set of strings, required) The command to execute\n"
                "\nAvailable commands:\n"
                "  count        - Get information about number of masternodes (DEPRECATED options: 'total', 'ps', 'enabled', 'qualify', 'all')\n"
                "  current      - Print info on current masternode winner to be paid the next block (calculated locally)\n"
                "  genkey       - Generate new masternodeprivkey\n"
#ifdef ENABLE_WALLET
                "  outputs      - Print masternode compatible outputs\n"
                "  start-alias  - Start single remote masternode by assigned alias configured in masternode.conf\n"
                "  start-<mode> - Start remote masternodes configured in masternode.conf (<mode>: 'all', 'missing', 'disabled')\n"
#endif // ENABLE_WALLET
                "  status       - Print masternode status information\n"
                "  list         - Print list of all known masternodes (see masternodelist for more info)\n"
                "  list-conf    - Print masternode.conf in JSON format\n"
                "  winner       - Print info on next masternode winner to vote for\n"
                "  winners      - Print list of masternode winners\n"
                );

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

    if (strCommand == "count")
    {
        if (request.params.size() > 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Too many parameters");

        int nCount;
        masternode_info_t mnInfo;
        mnodeman.GetNextMasternodeInQueueForPayment(true, nCount, mnInfo);

        int total = mnodeman.size();
        int ps = mnodeman.CountEnabled(MIN_PRIVATESEND_PEER_PROTO_VERSION);
        int enabled = mnodeman.CountEnabled();

        if (request.params.size() == 1) {
            UniValue obj(UniValue::VOBJ);

            obj.pushKV("total", total);
            obj.pushKV("ps_compatible", ps);
            obj.pushKV("enabled", enabled);
            obj.pushKV("qualify", nCount);

            return obj;
        }

        std::string strMode = request.params[1].get_str();

        if (strMode == "total")
            return total;

        if (strMode == "ps")
            return ps;

        if (strMode == "enabled")
            return enabled;

        if (strMode == "qualify")
            return nCount;

        if (strMode == "all")
            return strprintf("Total: %d (PS Compatible: %d / Enabled: %d / Qualify: %d)",
                total, ps, enabled, nCount);
    }

    if (strCommand == "current" || strCommand == "winner")
    {
        int nCount;
        int nHeight;
        masternode_info_t mnInfo;
        CBlockIndex* pindex = nullptr;
        {
            LOCK(cs_main);
            pindex = chainActive.Tip();
        }
        nHeight = pindex->nHeight + (strCommand == "current" ? 1 : 10);
        mnodeman.UpdateLastPaid(pindex);

        if(!mnodeman.GetNextMasternodeInQueueForPayment(nHeight, true, nCount, mnInfo))
            return "unknown";

        UniValue obj(UniValue::VOBJ);

        obj.pushKV("height",        nHeight);
        obj.pushKV("IP:port",       mnInfo.addr.ToString());
        obj.pushKV("protocol",      mnInfo.nProtocolVersion);
        obj.pushKV("outpoint",      mnInfo.outpoint.ToStringShort());
        obj.pushKV("payee",         EncodeDestination(mnInfo.collDest));
        obj.pushKV("lastseen",      mnInfo.nTimeLastPing);
        obj.pushKV("activeseconds", mnInfo.nTimeLastPing - mnInfo.sigTime);
        return obj;
    }

#ifdef ENABLE_WALLET
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (strCommand == "start-alias")
    {
        if (request.params.size() < 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Please specify an alias");

        {
            LOCK2(cs_main, pwallet ? &pwallet->cs_wallet : nullptr);
            EnsureWalletIsUnlocked(pwallet);
        }

        std::string strAlias = request.params[1].get_str();

        bool fFound = false;

        UniValue statusObj(UniValue::VOBJ);
        statusObj.pushKV("alias", strAlias);

        for (const auto& mne : masternodeConfig.getEntries()) {
            if(mne.getAlias() == strAlias) {
                fFound = true;
                std::string strError;
                CMasternodeBroadcast mnb;

                bool fResult = CMasternodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strError, mnb);

                int nDoS;
                if (fResult && !mnodeman.CheckMnbAndUpdateMasternodeList(nullptr, mnb, nDoS, g_connman.get())) {
                    strError = "Failed to verify MNB";
                    fResult = false;
                }

                statusObj.pushKV("result", fResult ? "successful" : "failed");
                if(!fResult) {
                    statusObj.pushKV("errorMessage", strError);
                }
                mnodeman.NotifyMasternodeUpdates(g_connman.get());
                break;
            }
        }

        if(!fFound) {
            statusObj.pushKV("result", "failed");
            statusObj.pushKV("errorMessage", "Could not find alias in config. Verify with list-conf.");
        }

        return statusObj;

    }

    if (strCommand == "start-all" || strCommand == "start-missing" || strCommand == "start-disabled")
    {
        {
            LOCK2(cs_main, pwallet ? &pwallet->cs_wallet : nullptr);
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

            COutPoint outpoint = COutPoint(uint256S(mne.getTxHash()), (uint32_t)atoi(mne.getOutputIndex()));
            CMasternode mn;
            bool fFound = mnodeman.Get(outpoint, mn);
            CMasternodeBroadcast mnb;

            if(strCommand == "start-missing" && fFound) continue;
            if(strCommand == "start-disabled" && fFound && mn.IsEnabled()) continue;

            bool fResult = CMasternodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strError, mnb);

            int nDoS;
            if (fResult && !mnodeman.CheckMnbAndUpdateMasternodeList(nullptr, mnb, nDoS, g_connman.get())) {
                strError = "Failed to verify MNB";
                fResult = false;
            }

            UniValue statusObj(UniValue::VOBJ);
            statusObj.pushKV("result", fResult ? "successful" : "failed");

            if (fResult) {
                nSuccessful++;
            } else {
                nFailed++;
                statusObj.pushKV("errorMessage", strError);
            }

            resultsObj.pushKV(mne.getAlias(), statusObj);
        }
        mnodeman.NotifyMasternodeUpdates(g_connman.get());

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
            COutPoint outpoint = COutPoint(uint256S(mne.getTxHash()), (uint32_t)atoi(mne.getOutputIndex()));
            CMasternode mn;
            bool fFound = mnodeman.Get(outpoint, mn);

            std::string strStatus = fFound ? mn.GetStatus() : "MISSING";

            UniValue mnObj(UniValue::VOBJ);
            mnObj.pushKV("address", mne.getIp());
            mnObj.pushKV("privateKey", mne.getPrivKey());
            mnObj.pushKV("txHash", mne.getTxHash());
            mnObj.pushKV("outputIndex", mne.getOutputIndex());
            mnObj.pushKV("status", strStatus);
            resultObj.pushKV(mne.getAlias(), mnObj);
        }

        return resultObj;
    }

#ifdef ENABLE_WALLET
    if (strCommand == "outputs") {
        // Find possible candidates
        std::vector<COutput> vPossibleCoins;
        LOCK2(cs_main, pwallet->cs_wallet);
        pwallet->AvailableCoins(vPossibleCoins, true, nullptr, ONLY_1000);

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
            mnObj.pushKV("payee", EncodeDestination(mn.collDest));
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
            nLast = atoi(request.params[1].get_str());
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

static std::string FormatVersion(int nVersion)
{
    if (nVersion % 100 == 0)
        return strprintf("%d.%d.%d", nVersion / 1000000, (nVersion / 10000) % 100, (nVersion / 100) % 100);
    else
        return strprintf("%d.%d.%d.%d", nVersion / 1000000, (nVersion / 10000) % 100, (nVersion / 100) % 100, nVersion % 100);
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
                "                   (since latest issued \"masternode start/start-many/start-alias\")\n"
                "  addr           - Print ip address associated with a masternode (can be additionally filtered, partial match)\n"
                "  daemon         - Print daemon version of a masternode (can be additionally filtered, exact match)\n"
                "  full           - Print info in format 'status protocol payee lastseen activeseconds lastpaidtime lastpaidblock IP'\n"
                "                   (can be additionally filtered, partial match)\n"
                "  info           - Print info in format 'status protocol payee lastseen activeseconds sentinelversion sentinelstate IP'\n"
                "                   (can be additionally filtered, partial match)\n"
                "  json           - Print info in JSON format (can be additionally filtered, partial match)\n"
                "  lastpaidblock  - Print the last block height a node was paid on the network\n"
                "  lastpaidtime   - Print the last time a node was paid on the network\n"
                "  lastseen       - Print timestamp of when a masternode was last seen on the network\n"
                "  payee          - Print Chaincoin address associated with a masternode (can be additionally filtered,\n"
                "                   partial match)\n"
                "  protocol       - Print protocol of a masternode (can be additionally filtered, exact match)\n"
                "  pubkey         - Print the masternode (not collateral) public key\n"
                "  rank           - Print rank of a masternode based on current block\n"
                "  sentinel       - Print sentinel version of a masternode (can be additionally filtered, exact match)\n"
                "  status         - Print masternode status: PRE_ENABLED / ENABLED / EXPIRED / SENTINEL_PING_EXPIRED / NEW_START_REQUIRED /\n"
                "                   UPDATE_REQUIRED / POSE_BAN / OUTPOINT_SPENT (can be additionally filtered, partial match)\n"
                );
    }

    if (strMode == "full" || strMode == "json" || strMode == "lastpaidtime" || strMode == "lastpaidblock") {
        CBlockIndex* pindex = nullptr;
        {
            LOCK(cs_main);
            pindex = chainActive.Tip();
        }
        mnodeman.UpdateLastPaid(pindex);
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
        std::map<COutPoint, CMasternode> mapMasternodes = mnodeman.GetFullMasternodeMap();
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
                std::string strDaemon = mn.lastPing.nDaemonVersion > 0 ? FormatVersion(mn.lastPing.nDaemonVersion) : "Unknown";
                if (strFilter !="" && strDaemon.find(strFilter) == std::string::npos &&
                    strOutpoint.find(strFilter) == std::string::npos) continue;
                obj.pushKV(strOutpoint, strDaemon);
            } else if (strMode == "sentinel") {
                std::string strSentinel = mn.lastPing.nSentinelVersion > 0 ? SafeIntVersionToString(mn.lastPing.nSentinelVersion) : "Unknown";
                if (strFilter !="" && strSentinel.find(strFilter) == std::string::npos &&
                    strOutpoint.find(strFilter) == std::string::npos) continue;
                obj.pushKV(strOutpoint, strSentinel);
            } else if (strMode == "full") {
                std::ostringstream streamFull;
                streamFull << std::setw(18) <<
                               mn.GetStatus() << " " <<
                               mn.nProtocolVersion << " " <<
                               EncodeDestination(mn.collDest) << " " <<
                               (int64_t)mn.lastPing.sigTime << " " << std::setw(8) <<
                               (int64_t)(mn.lastPing.sigTime - mn.sigTime) << " " << std::setw(10) <<
                               mn.GetLastPaidTime() << " "  << std::setw(6) <<
                               mn.GetLastPaidBlock() << " " <<
                               mn.addr.ToString();
                std::string strFull = streamFull.str();
                if (strFilter !="" && strFull.find(strFilter) == std::string::npos &&
                    strOutpoint.find(strFilter) == std::string::npos) continue;
                obj.pushKV(strOutpoint, strFull);
            } else if (strMode == "info") {
                std::ostringstream streamInfo;
                streamInfo << std::setw(18) <<
                               mn.GetStatus() << " " <<
                               mn.nProtocolVersion << " " <<
                               EncodeDestination(mn.collDest) << " " <<
                               (int64_t)mn.lastPing.sigTime << " " << std::setw(8) <<
                               (int64_t)(mn.lastPing.sigTime - mn.sigTime) << " " <<
                               SafeIntVersionToString(mn.lastPing.nSentinelVersion) << " "  <<
                               (mn.lastPing.fSentinelIsCurrent ? "current" : "expired") << " " <<
                               mn.addr.ToString();
                std::string strInfo = streamInfo.str();
                if (strFilter !="" && strInfo.find(strFilter) == std::string::npos &&
                    strOutpoint.find(strFilter) == std::string::npos) continue;
                obj.pushKV(strOutpoint, strInfo);
            } else if (strMode == "json") {
                std::ostringstream streamInfo;
                streamInfo <<  mn.addr.ToString() << " " <<
                               EncodeDestination(mn.collDest) << " " <<
                               mn.GetStatus() << " " <<
                               mn.nProtocolVersion << " " <<
                               mn.lastPing.nDaemonVersion << " " <<
                               SafeIntVersionToString(mn.lastPing.nSentinelVersion) << " " <<
                               (mn.lastPing.fSentinelIsCurrent ? "current" : "expired") << " " <<
                               (int64_t)mn.lastPing.sigTime << " " <<
                               (int64_t)(mn.lastPing.sigTime - mn.sigTime) << " " <<
                               mn.GetLastPaidTime() << " " <<
                               mn.GetLastPaidBlock();
                std::string strInfo = streamInfo.str();
                if (strFilter !="" && strInfo.find(strFilter) == std::string::npos &&
                    strOutpoint.find(strFilter) == std::string::npos) continue;
                UniValue objMN(UniValue::VOBJ);
                objMN.pushKV("address", mn.addr.ToString());
                objMN.pushKV("payee", EncodeDestination(mn.collDest));
                objMN.pushKV("status", mn.GetStatus());
                objMN.pushKV("protocol", mn.nProtocolVersion);
                objMN.pushKV("daemonversion", mn.lastPing.nDaemonVersion > 0 ? FormatVersion(mn.lastPing.nDaemonVersion) : "Unknown");
                objMN.pushKV("sentinelversion", mn.lastPing.nSentinelVersion > 0 ? SafeIntVersionToString(mn.lastPing.nSentinelVersion) : "Unknown");
                objMN.pushKV("sentinelstate", (mn.lastPing.fSentinelIsCurrent ? "current" : "expired"));
                objMN.pushKV("lastseen", (int64_t)mn.lastPing.sigTime);
                objMN.pushKV("activeseconds", (int64_t)(mn.lastPing.sigTime - mn.sigTime));
                objMN.pushKV("lastpaidtime", mn.GetLastPaidTime());
                objMN.pushKV("lastpaidblock", mn.GetLastPaidBlock());
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
                std::string strPayee = EncodeDestination(mn.collDest);
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
    CDataStream ssData(mnbData, SER_NETWORK, PROTOCOL_VERSION);
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
    std::string strCommand;
    if (request.params.size() >= 1)
        strCommand = request.params[0].get_str();

    if (request.fHelp  ||
        (
#ifdef ENABLE_WALLET
            strCommand != "create-alias" && strCommand != "create-all" &&
#endif // ENABLE_WALLET
            strCommand != "decode" && strCommand != "relay"))
        throw std::runtime_error(
                "masternodebroadcast \"command\"...\n"
                "Set of commands to create and relay masternode broadcast messages\n"
                "\nArguments:\n"
                "1. \"command\"        (string or set of strings, required) The command to execute\n"
                "\nAvailable commands:\n"
#ifdef ENABLE_WALLET
                "  create-alias  - Create single remote masternode broadcast message by assigned alias configured in masternode.conf\n"
                "  create-all    - Create remote masternode broadcast messages for all masternodes configured in masternode.conf\n"
#endif // ENABLE_WALLET
                "  decode        - Decode masternode broadcast message\n"
                "  relay         - Relay masternode broadcast message to the network\n"
                );

#ifdef ENABLE_WALLET
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (strCommand == "create-alias")
    {
        // wait for reindex and/or import to finish
        if (fImporting || fReindex)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Wait for reindex and/or import to finish");

        if (request.params.size() < 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Please specify an alias");

        {
            LOCK2(cs_main, pwallet ? &pwallet->cs_wallet : nullptr);
            EnsureWalletIsUnlocked(pwallet);
        }

        bool fFound = false;
        std::string strAlias = request.params[1].get_str();

        UniValue statusObj(UniValue::VOBJ);
        std::vector<CMasternodeBroadcast> vecMnb;

        statusObj.pushKV("alias", strAlias);

        for (const auto& mne : masternodeConfig.getEntries()) {
            if(mne.getAlias() == strAlias) {
                fFound = true;
                std::string strError;
                CMasternodeBroadcast mnb;

                bool fResult = CMasternodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strError, mnb, true);

                statusObj.pushKV("result", fResult ? "successful" : "failed");
                if(fResult) {
                    vecMnb.push_back(mnb);
                    CDataStream ssVecMnb(SER_NETWORK, PROTOCOL_VERSION);
                    ssVecMnb << vecMnb;
                    statusObj.pushKV("hex", HexStr(ssVecMnb.begin(), ssVecMnb.end()));
                } else {
                    statusObj.pushKV("errorMessage", strError);
                }
                break;
            }
        }

        if(!fFound) {
            statusObj.pushKV("result", "not found");
            statusObj.pushKV("errorMessage", "Could not find alias in config. Verify with list-conf.");
        }

        return statusObj;

    }

    if (strCommand == "create-all")
    {
        // wait for reindex and/or import to finish
        if (fImporting || fReindex)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Wait for reindex and/or import to finish");

        {
            LOCK2(cs_main, pwallet ? &pwallet->cs_wallet : nullptr);
            EnsureWalletIsUnlocked(pwallet);
        }

        int nSuccessful = 0;
        int nFailed = 0;

        UniValue resultsObj(UniValue::VOBJ);
        std::vector<CMasternodeBroadcast> vecMnb;

        for (const auto& mne : masternodeConfig.getEntries()) {
            std::string strError;
            CMasternodeBroadcast mnb;

            bool fResult = CMasternodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strError, mnb, true);

            UniValue statusObj(UniValue::VOBJ);
            statusObj.pushKV("alias", mne.getAlias());
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

        CDataStream ssVecMnb(SER_NETWORK, PROTOCOL_VERSION);
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
                resultObj.pushKV("pubKeyCollateralAddress", EncodeDestination(mnb.collDest));
                resultObj.pushKV("pubKeyMasternode", EncodeDestination(mnb.pubKeyMasternode.GetID()));
                resultObj.pushKV("vchSig", EncodeBase64(&mnb.vchSig[0], mnb.vchSig.size()));
                resultObj.pushKV("sigTime", mnb.sigTime);
                resultObj.pushKV("protocolVersion", mnb.nProtocolVersion);
                resultObj.pushKV("nLastDsq", mnb.nLastDsq);

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
                fResult = mnodeman.CheckMnbAndUpdateMasternodeList(nullptr, mnb, nDos, g_connman.get());
                mnodeman.NotifyMasternodeUpdates(g_connman.get());
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

static UniValue sentinelping(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            "sentinelping version\n"
            "\nSentinel ping.\n"
            "\nArguments:\n"
            "1. version           (string, required) Sentinel version in the form \"x.x.x\"\n"
            "\nResult:\n"
            "state                (boolean) Ping result\n"
            "\nExamples:\n"
            + HelpExampleCli("sentinelping", "1.0.2")
            + HelpExampleRpc("sentinelping", "1.0.2")
        );
    }

    activeMasternode.UpdateSentinelPing(StringVersionToInt(request.params[0].get_str()));
    return true;
}

static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         argNames
  //  --------------------- ------------------------  -----------------------  ----------
    { "chaincoin",          "masternode",             &masternode,             {} },
    { "chaincoin",          "masternodelist",         &masternodelist,         {} },
    { "chaincoin",          "masternodebroadcast",    &masternodebroadcast,    {} },
    { "chaincoin",          "getpoolinfo",            &getpoolinfo,            {} },
    { "chaincoin",          "sentinelping",           &sentinelping,           {} },
#ifdef ENABLE_WALLET
    { "chaincoin",          "privatesend",            &privatesend,            {} },
#endif // ENABLE_WALLET
};

void RegisterMasternodeRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
