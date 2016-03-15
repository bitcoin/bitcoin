// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "db.h"
#include "init.h"
#include "activemasternode.h"
#include "masternode-budget.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "masternodeconfig.h"
#include "masternodeman.h"
#include "rpcserver.h"
#include "utilmoneystr.h"

#include <fstream>
#include <iomanip>
#include <univalue.h>

UniValue darksend(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "darksend \"command\"\n"
            "\nArguments:\n"
            "1. \"command\"        (string or set of strings, required) The command to execute\n"
            "\nAvailable commands:\n"
            "  start       - Start mixing\n"
            "  stop        - Stop mixing\n"
            "  reset       - Reset mixing\n"
            "  status      - Print mixing status\n"
            + HelpRequiringPassphrase());

    if(params[0].get_str() == "start"){
        if (pwalletMain->IsLocked())
            throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");

        if(fMasterNode)
            return "Mixing is not supported from masternodes";

        fEnableDarksend = true;
        bool result = darkSendPool.DoAutomaticDenominating();
//        fEnableDarksend = result;
        return "Mixing " + (result ? "started successfully" : ("start failed: " + darkSendPool.GetStatus() + ", will retry"));
    }

    if(params[0].get_str() == "stop"){
        fEnableDarksend = false;
        return "Mixing was stopped";
    }

    if(params[0].get_str() == "reset"){
        darkSendPool.Reset();
        return "Mixing was reset";
    }

    if(params[0].get_str() == "status"){
        return "Mixing status: " + darkSendPool.GetStatus();
    }

    return "Unknown command, please see \"help darksend\"";
}

UniValue getpoolinfo(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getpoolinfo\n"
            "Returns an object containing anonymous pool-related information.");

    UniValue obj(UniValue::VOBJ);
    if (darkSendPool.pSubmittedToMasternode)
        obj.push_back(Pair("masternode",        darkSendPool.pSubmittedToMasternode->addr.ToString()));
    obj.push_back(Pair("queue",                 (int64_t)vecDarksendQueue.size()));
    obj.push_back(Pair("state",                 darkSendPool.GetState()));
    obj.push_back(Pair("entries",               darkSendPool.GetEntriesCount()));
    obj.push_back(Pair("entries_accepted",      darkSendPool.GetCountEntriesAccepted()));
    return obj;
}


UniValue masternode(const UniValue& params, bool fHelp)
{
    string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();

    if (fHelp  ||
        (strCommand != "start" && strCommand != "start-alias" && strCommand != "start-many" && strCommand != "start-all" && strCommand != "start-missing" &&
         strCommand != "start-disabled" && strCommand != "list" && strCommand != "list-conf" && strCommand != "count"  && strCommand != "enforce" &&
        strCommand != "debug" && strCommand != "current" && strCommand != "winners" && strCommand != "genkey" && strCommand != "connect" &&
        strCommand != "outputs" && strCommand != "status" && strCommand != "calcscore"))
        throw runtime_error(
                "masternode \"command\"... ( \"passphrase\" )\n"
                "Set of commands to execute masternode related actions\n"
                "\nArguments:\n"
                "1. \"command\"        (string or set of strings, required) The command to execute\n"
                "2. \"passphrase\"     (string, optional) The wallet passphrase\n"
                "\nAvailable commands:\n"
                "  count        - Print number of all known masternodes (optional: 'ds', 'enabled', 'all', 'qualify')\n"
                "  current      - Print info on current masternode winner\n"
                "  debug        - Print masternode status\n"
                "  genkey       - Generate new masternodeprivkey\n"
                "  enforce      - Enforce masternode payments\n"
                "  outputs      - Print masternode compatible outputs\n"
                "  start        - Start local Hot masternode configured in dash.conf\n"
                "  start-alias  - Start single remote masternode by assigned alias configured in masternode.conf\n"
                "  start-<mode> - Start remote masternodes configured in masternode.conf (<mode>: 'all', 'missing', 'disabled')\n"
                "  status       - Print masternode status information\n"
                "  list         - Print list of all known masternodes (see masternodelist for more info)\n"
                "  list-conf    - Print masternode.conf in JSON format\n"
                "  winners      - Print list of masternode winners\n"
                );

    if (strCommand == "list")
    {
        UniValue newParams(UniValue::VARR);
        // forward params but skip "list"
        for (unsigned int i = 1; i < params.size(); i++)
            newParams.push_back(params[i]);
        return masternodelist(newParams, fHelp);
    }

    if(strCommand == "connect")
    {
        std::string strAddress = "";
        if (params.size() == 2){
            strAddress = params[1].get_str();
        } else {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Masternode address required");
        }

        CService addr = CService(strAddress);

        CNode *pnode = ConnectNode((CAddress)addr, NULL, false);
        if(pnode){
            pnode->Release();
            return "successfully connected";
        } else {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Error connecting");
        }
    }

    if (strCommand == "count")
    {
        if (params.size() > 2){
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Too many parameters");
        }
        if (params.size() == 2)
        {
            int nCount = 0;

            {
                LOCK(cs_main);
                if(chainActive.Tip())
                    mnodeman.GetNextMasternodeInQueueForPayment(chainActive.Tip()->nHeight, true, nCount);
            }

            if(params[1].get_str() == "ds") return mnodeman.CountEnabled(MIN_POOL_PEER_PROTO_VERSION);
            if(params[1].get_str() == "enabled") return mnodeman.CountEnabled();
            if(params[1].get_str() == "qualify") return nCount;
            if(params[1].get_str() == "all") return strprintf("Total: %d (DS Compatible: %d / Enabled: %d / Qualify: %d)",
                                                    mnodeman.size(),
                                                    mnodeman.CountEnabled(MIN_POOL_PEER_PROTO_VERSION),
                                                    mnodeman.CountEnabled(),
                                                    nCount);
        }
        return mnodeman.size();
    }

    if (strCommand == "current")
    {
        int nCount = 0;
        LOCK(cs_main);
        CMasternode* winner = NULL;
        if(chainActive.Tip())
            winner = mnodeman.GetNextMasternodeInQueueForPayment(chainActive.Height() - 100, true, nCount);
        if(winner) {
            UniValue obj(UniValue::VOBJ);

            obj.push_back(Pair("IP:port",       winner->addr.ToString()));
            obj.push_back(Pair("protocol",      (int64_t)winner->protocolVersion));
            obj.push_back(Pair("vin",           winner->vin.prevout.ToStringShort()));
            obj.push_back(Pair("pubkey",        CBitcoinAddress(winner->pubkey.GetID()).ToString()));
            obj.push_back(Pair("lastseen",      (winner->lastPing == CMasternodePing()) ? winner->sigTime :
                                                        winner->lastPing.sigTime));
            obj.push_back(Pair("activeseconds", (winner->lastPing == CMasternodePing()) ? 0 :
                                                        (winner->lastPing.sigTime - winner->sigTime)));
            return obj;
        }

        return "unknown";
    }

    if (strCommand == "debug")
    {
        if(activeMasternode.status != ACTIVE_MASTERNODE_INITIAL || !masternodeSync.IsBlockchainSynced())
            return activeMasternode.GetStatus();

        CTxIn vin = CTxIn();
        CPubKey pubkey = CPubKey();
        CKey key;
        bool found = activeMasternode.GetMasterNodeVin(vin, pubkey, key);
        if(!found){
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Missing masternode input, please look at the documentation for instructions on masternode creation");
        } else {
            return activeMasternode.GetStatus();
        }
    }

    if(strCommand == "enforce")
    {
        return (uint64_t)enforceMasternodePaymentsTime;
    }

    if (strCommand == "start")
    {
        if(!fMasterNode)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "You must set masternode=1 in the configuration");

        if(pwalletMain->IsLocked()) {
            SecureString strWalletPass;
            strWalletPass.reserve(100);

            if (params.size() == 2){
                strWalletPass = params[1].get_str().c_str();
            } else {
                throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Your wallet is locked, passphrase is required");
            }

            if(!pwalletMain->Unlock(strWalletPass)){
                throw JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "The wallet passphrase entered was incorrect");
            }
        }

        if(activeMasternode.status != ACTIVE_MASTERNODE_STARTED){
            activeMasternode.status = ACTIVE_MASTERNODE_INITIAL; // TODO: consider better way
            activeMasternode.ManageStatus();
            pwalletMain->Lock();
        }

        return activeMasternode.GetStatus();
    }

    if (strCommand == "start-alias")
    {
        if (params.size() < 2){
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Command needs at least 2 parameters");
        }

        std::string alias = params[1].get_str();

        if(pwalletMain->IsLocked()) {
            SecureString strWalletPass;
            strWalletPass.reserve(100);

            if (params.size() == 3){
                strWalletPass = params[2].get_str().c_str();
            } else {
                throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Your wallet is locked, passphrase is required");
            }

            if(!pwalletMain->Unlock(strWalletPass)){
                throw JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "The wallet passphrase entered was incorrect");
            }
        }

        bool found = false;

        UniValue statusObj(UniValue::VOBJ);
        statusObj.push_back(Pair("alias", alias));

        BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
            if(mne.getAlias() == alias) {
                found = true;
                std::string errorMessage;

                bool result = activeMasternode.Register(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), errorMessage);

                statusObj.push_back(Pair("result", result ? "successful" : "failed"));
                if(!result) {
                    statusObj.push_back(Pair("errorMessage", errorMessage));
                }
                break;
            }
        }

        if(!found) {
            statusObj.push_back(Pair("result", "failed"));
            statusObj.push_back(Pair("errorMessage", "Could not find alias in config. Verify with list-conf."));
        }

        pwalletMain->Lock();
        return statusObj;

    }

    if (strCommand == "start-many" || strCommand == "start-all" || strCommand == "start-missing" || strCommand == "start-disabled")
    {
        if(pwalletMain->IsLocked()) {
            SecureString strWalletPass;
            strWalletPass.reserve(100);

            if (params.size() == 2){
                strWalletPass = params[1].get_str().c_str();
            } else {
                throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Your wallet is locked, passphrase is required");
            }

            if(!pwalletMain->Unlock(strWalletPass)){
                throw JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "The wallet passphrase entered was incorrect");
            }
        }

        if((strCommand == "start-missing" || strCommand == "start-disabled") &&
         (masternodeSync.RequestedMasternodeAssets <= MASTERNODE_SYNC_LIST ||
          masternodeSync.RequestedMasternodeAssets == MASTERNODE_SYNC_FAILED)) {
            throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "You can't use this command until masternode list is synced");
        }

        std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        int successful = 0;
        int failed = 0;

        UniValue resultsObj(UniValue::VOBJ);

        BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
            std::string errorMessage;

            CTxIn vin = CTxIn(uint256S(mne.getTxHash()), uint32_t(atoi(mne.getOutputIndex().c_str())));
            CMasternode *pmn = mnodeman.Find(vin);

            if(strCommand == "start-missing" && pmn) continue;
            if(strCommand == "start-disabled" && pmn && pmn->IsEnabled()) continue;

            bool result = activeMasternode.Register(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), errorMessage);

            UniValue statusObj(UniValue::VOBJ);
            statusObj.push_back(Pair("alias", mne.getAlias()));
            statusObj.push_back(Pair("result", result ? "successful" : "failed"));

            if(result) {
                successful++;
            } else {
                failed++;
                statusObj.push_back(Pair("errorMessage", errorMessage));
            }

            resultsObj.push_back(Pair("status", statusObj));
        }
        pwalletMain->Lock();

        UniValue returnObj(UniValue::VOBJ);
        returnObj.push_back(Pair("overall", strprintf("Successfully started %d masternodes, failed to start %d, total %d", successful, failed, successful + failed)));
        returnObj.push_back(Pair("detail", resultsObj));

        return returnObj;
    }

    if (strCommand == "genkey")
    {
        CKey secret;
        secret.MakeNewKey(false);

        return CBitcoinSecret(secret).ToString();
    }

    if(strCommand == "list-conf")
    {
        std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        UniValue resultObj(UniValue::VOBJ);

        BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
            CTxIn vin = CTxIn(uint256S(mne.getTxHash()), uint32_t(atoi(mne.getOutputIndex().c_str())));
            CMasternode *pmn = mnodeman.Find(vin);

            std::string strStatus = pmn ? pmn->Status() : "MISSING";

            UniValue mnObj(UniValue::VOBJ);
            mnObj.push_back(Pair("alias", mne.getAlias()));
            mnObj.push_back(Pair("address", mne.getIp()));
            mnObj.push_back(Pair("privateKey", mne.getPrivKey()));
            mnObj.push_back(Pair("txHash", mne.getTxHash()));
            mnObj.push_back(Pair("outputIndex", mne.getOutputIndex()));
            mnObj.push_back(Pair("status", strStatus));
            resultObj.push_back(Pair("masternode", mnObj));
        }

        return resultObj;
    }

    if (strCommand == "outputs"){
        // Find possible candidates
        vector<COutput> possibleCoins = activeMasternode.SelectCoinsMasternode();

        UniValue obj(UniValue::VOBJ);
        BOOST_FOREACH(COutput& out, possibleCoins) {
            obj.push_back(Pair(out.tx->GetHash().ToString(), strprintf("%d", out.i)));
        }

        return obj;

    }

    if(strCommand == "status")
    {
        if(!fMasterNode)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "This is not a masternode");

        UniValue mnObj(UniValue::VOBJ);
        CMasternode *pmn = mnodeman.Find(activeMasternode.vin);

        mnObj.push_back(Pair("vin", activeMasternode.vin.ToString()));
        mnObj.push_back(Pair("service", activeMasternode.service.ToString()));
        if (pmn) mnObj.push_back(Pair("pubkey", CBitcoinAddress(pmn->pubkey.GetID()).ToString()));
        mnObj.push_back(Pair("status", activeMasternode.GetStatus()));
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

        if (params.size() >= 2){
            nLast = atoi(params[1].get_str());
        }

        if (params.size() == 3){
            strFilter = params[2].get_str();
        }

        if (params.size() > 3)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'masternode winners ( \"count\" \"filter\" )'");

        UniValue obj(UniValue::VOBJ);

        for(int i = nHeight - nLast; i < nHeight + 20; i++)
        {
            std::string strPayment = GetRequiredPaymentsString(i);
            if(strFilter !="" && strPayment.find(strFilter) == string::npos) continue;
            obj.push_back(Pair(strprintf("%d", i), strPayment));
        }

        return obj;
    }

    /*
        Shows which masternode wins by score each block
    */
    if (strCommand == "calcscore")
    {

        int nHeight;
        {
            LOCK(cs_main);
            CBlockIndex* pindex = chainActive.Tip();
            if(!pindex) return NullUniValue;

            nHeight = pindex->nHeight;
        }

        int nLast = 10;

        if (params.size() >= 2){
            nLast = atoi(params[1].get_str());
        }
        UniValue obj(UniValue::VOBJ);

        std::vector<CMasternode> vMasternodes = mnodeman.GetFullMasternodeVector();
        for(int i = nHeight - nLast; i < nHeight + 20; i++){
            arith_uint256 nHigh = 0;
            CMasternode *pBestMasternode = NULL;
            BOOST_FOREACH(CMasternode& mn, vMasternodes) {
                arith_uint256 n = UintToArith256(mn.CalculateScore(1, i - 100));
                if(n > nHigh){
                    nHigh = n;
                    pBestMasternode = &mn;
                }
            }
            if(pBestMasternode)
                obj.push_back(Pair(strprintf("%d", i), pBestMasternode->vin.prevout.ToStringShort().c_str()));
        }

        return obj;
    }

    return NullUniValue;
}

UniValue masternodelist(const UniValue& params, bool fHelp)
{
    std::string strMode = "status";
    std::string strFilter = "";

    if (params.size() >= 1) strMode = params[0].get_str();
    if (params.size() == 2) strFilter = params[1].get_str();

    if (fHelp ||
            (strMode != "status" && strMode != "vin" && strMode != "pubkey" && strMode != "lastseen" && strMode != "activeseconds" && strMode != "rank" && strMode != "addr"
                && strMode != "protocol" && strMode != "full" && strMode != "lastpaid"))
    {
        throw runtime_error(
                "masternodelist ( \"mode\" \"filter\" )\n"
                "Get a list of masternodes in different modes\n"
                "\nArguments:\n"
                "1. \"mode\"      (string, optional/required to use filter, defaults = status) The mode to run list in\n"
                "2. \"filter\"    (string, optional) Filter results. Partial match by IP by default in all modes,\n"
                "                                    additional matches in some modes are also available\n"
                "\nAvailable modes:\n"
                "  activeseconds  - Print number of seconds masternode recognized by the network as enabled\n"
                "                   (since latest issued \"masternode start/start-many/start-alias\")\n"
                "  addr           - Print ip address associated with a masternode (can be additionally filtered, partial match)\n"
                "  full           - Print info in format 'status protocol pubkey IP lastseen activeseconds lastpaid'\n"
                "                   (can be additionally filtered, partial match)\n"
                "  lastseen       - Print timestamp of when a masternode was last seen on the network\n"
                "  lastpaid       - The last time a node was paid on the network\n"
                "  protocol       - Print protocol of a masternode (can be additionally filtered, exact match))\n"
                "  pubkey         - Print public key associated with a masternode (can be additionally filtered,\n"
                "                   partial match)\n"
                "  rank           - Print rank of a masternode based on current block\n"
                "  status         - Print masternode status: ENABLED / EXPIRED / VIN_SPENT / REMOVE / POS_ERROR\n"
                "                   (can be additionally filtered, partial match)\n"
                );
    }

    UniValue obj(UniValue::VOBJ);
    if (strMode == "rank") {
        std::vector<pair<int, CMasternode> > vMasternodeRanks = mnodeman.GetMasternodeRanks(chainActive.Tip()->nHeight);
        BOOST_FOREACH(PAIRTYPE(int, CMasternode)& s, vMasternodeRanks) {
            std::string strVin = s.second.vin.prevout.ToStringShort();
            if(strFilter !="" && strVin.find(strFilter) == string::npos) continue;
            obj.push_back(Pair(strVin,       s.first));
        }
    } else {
        std::vector<CMasternode> vMasternodes = mnodeman.GetFullMasternodeVector();
        BOOST_FOREACH(CMasternode& mn, vMasternodes) {
            std::string strVin = mn.vin.prevout.ToStringShort();
            if (strMode == "activeseconds") {
                if(strFilter !="" && strVin.find(strFilter) == string::npos) continue;
                obj.push_back(Pair(strVin,       (int64_t)(mn.lastPing.sigTime - mn.sigTime)));
            } else if (strMode == "addr") {
                if(strFilter !="" && mn.vin.prevout.hash.ToString().find(strFilter) == string::npos &&
                    strVin.find(strFilter) == string::npos) continue;
                obj.push_back(Pair(strVin,       mn.addr.ToString()));
            } else if (strMode == "full") {
                std::ostringstream addrStream;
                addrStream << setw(21) << strVin;

                std::ostringstream stringStream;
                stringStream << setw(9) <<
                               mn.Status() << " " <<
                               mn.protocolVersion << " " <<
                               CBitcoinAddress(mn.pubkey.GetID()).ToString() << " " << setw(21) <<
                               mn.addr.ToString() << " " <<
                               (int64_t)mn.lastPing.sigTime << " " << setw(8) <<
                               (int64_t)(mn.lastPing.sigTime - mn.sigTime) << " " <<
                               (int64_t)mn.GetLastPaid();
                std::string output = stringStream.str();
                stringStream << " " << strVin;
                if(strFilter !="" && stringStream.str().find(strFilter) == string::npos &&
                        strVin.find(strFilter) == string::npos) continue;
                obj.push_back(Pair(addrStream.str(), output));
            } else if (strMode == "lastseen") {
                if(strFilter !="" && strVin.find(strFilter) == string::npos) continue;
                obj.push_back(Pair(strVin,       (int64_t)mn.lastPing.sigTime));
            } else if (strMode == "lastpaid"){
                if(strFilter !="" && mn.vin.prevout.hash.ToString().find(strFilter) == string::npos &&
                    strVin.find(strFilter) == string::npos) continue;
                obj.push_back(Pair(strVin,      (int64_t)mn.GetLastPaid()));
            } else if (strMode == "protocol") {
                if(strFilter !="" && strFilter != strprintf("%d", mn.protocolVersion) &&
                    strVin.find(strFilter) == string::npos) continue;
                obj.push_back(Pair(strVin,       (int64_t)mn.protocolVersion));
            } else if (strMode == "pubkey") {
                CBitcoinAddress address(mn.pubkey.GetID());

                if(strFilter !="" && address.ToString().find(strFilter) == string::npos &&
                    strVin.find(strFilter) == string::npos) continue;
                obj.push_back(Pair(strVin,       address.ToString()));
            } else if(strMode == "status") {
                std::string strStatus = mn.Status();
                if(strFilter !="" && strVin.find(strFilter) == string::npos && strStatus.find(strFilter) == string::npos) continue;
                obj.push_back(Pair(strVin,       strStatus));
            }
        }
    }
    return obj;

}
