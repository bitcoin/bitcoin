// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "db.h"
#include "init.h"
#include "activemasternode.h"
#include "darksend.h"
#include "governance.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "masternodeconfig.h"
#include "masternodeman.h"
#include "rpcserver.h"
#include "utilmoneystr.h"

#include <fstream>
#include <iomanip>
#include <univalue.h>

void EnsureWalletIsUnlocked();

UniValue privatesend(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "privatesend \"command\"\n"
            "\nArguments:\n"
            "1. \"command\"        (string or set of strings, required) The command to execute\n"
            "\nAvailable commands:\n"
            "  start       - Start mixing\n"
            "  stop        - Stop mixing\n"
            "  reset       - Reset mixing\n"
            "  status      - Print mixing status\n"
            + HelpRequiringPassphrase());

    if(params[0].get_str() == "start") {
        if (pwalletMain->IsLocked(true))
            throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");

        if(fMasterNode)
            return "Mixing is not supported from masternodes";

        fEnablePrivateSend = true;
        bool result = darkSendPool.DoAutomaticDenominating();
        return "Mixing " + (result ? "started successfully" : ("start failed: " + darkSendPool.GetStatus() + ", will retry"));
    }

    if(params[0].get_str() == "stop"){
        fEnablePrivateSend = false;
        return "Mixing was stopped";
    }

    if(params[0].get_str() == "reset"){
        darkSendPool.ResetPool();
        return "Mixing was reset";
    }

    if(params[0].get_str() == "status"){
        UniValue obj(UniValue::VOBJ);
        obj.push_back(Pair("status",            darkSendPool.GetStatus()));
        obj.push_back(Pair("keys_left",     pwalletMain->nKeysLeftSinceAutoBackup));
        obj.push_back(Pair("warnings",      (pwalletMain->nKeysLeftSinceAutoBackup < PRIVATESEND_KEYS_THRESHOLD_WARNING
                                                ? "WARNING: keypool is almost depleted!" : "")));
        return obj;
    }

    return "Unknown command, please see \"help privatesend\"";
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
    obj.push_back(Pair("queue",                 darkSendPool.GetQueueSize()));
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
                "  count        - Print number of all known masternodes (optional: 'ps', 'enabled', 'all', 'qualify')\n"
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

        CNode *pnode = ConnectNode((CAddress)addr, NULL);
        if(pnode) {
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

            if(params[1].get_str() == "ps") return mnodeman.CountEnabled(MIN_PRIVATESEND_PEER_PROTO_VERSION);
            if(params[1].get_str() == "enabled") return mnodeman.CountEnabled();
            if(params[1].get_str() == "qualify") return nCount;
            if(params[1].get_str() == "all") return strprintf("Total: %d (PS Compatible: %d / Enabled: %d / Qualify: %d)",
                                                    mnodeman.size(),
                                                    mnodeman.CountEnabled(MIN_PRIVATESEND_PEER_PROTO_VERSION),
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
        if(activeMasternode.nState != ACTIVE_MASTERNODE_INITIAL || !masternodeSync.IsBlockchainSynced())
            return activeMasternode.GetStatus();

        CTxIn vin = CTxIn();
        CPubKey pubkey = CPubKey();
        CKey key;
        if(!pwalletMain || !pwalletMain->GetMasternodeVinAndKeys(vin, pubkey, key))
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Missing masternode input, please look at the documentation for instructions on masternode creation");

        return activeMasternode.GetStatus();
    }

    if(strCommand == "enforce")
    {
        return (uint64_t)enforceMasternodePaymentsTime;
    }

    if (strCommand == "start")
    {
        if(!fMasterNode)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "You must set masternode=1 in the configuration");

        LOCK(pwalletMain->cs_wallet);
        EnsureWalletIsUnlocked();

        if(activeMasternode.nState != ACTIVE_MASTERNODE_STARTED){
            activeMasternode.nState = ACTIVE_MASTERNODE_INITIAL; // TODO: consider better way
            activeMasternode.ManageState();
        }

        return activeMasternode.GetStatus();
    }

    if (strCommand == "start-alias")
    {
        if (params.size() < 2){
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Command needs at least 2 parameters");
        }

        std::string alias = params[1].get_str();


        LOCK(pwalletMain->cs_wallet);
        EnsureWalletIsUnlocked();


        bool found = false;

        UniValue statusObj(UniValue::VOBJ);
        statusObj.push_back(Pair("alias", alias));

        BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
            if(mne.getAlias() == alias) {
                found = true;
                std::string errorMessage;
                CMasternodeBroadcast mnb;

                bool result = CMasternodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), errorMessage, mnb);

                statusObj.push_back(Pair("result", result ? "successful" : "failed"));
                if(result) {
                    mnodeman.UpdateMasternodeList(mnb);
                    mnb.Relay();
                } else {
                    statusObj.push_back(Pair("errorMessage", errorMessage));
                }
                break;
            }
        }

        if(!found) {
            statusObj.push_back(Pair("result", "failed"));
            statusObj.push_back(Pair("errorMessage", "Could not find alias in config. Verify with list-conf."));
        }

        return statusObj;

    }

    if (strCommand == "start-many" || strCommand == "start-all" || strCommand == "start-missing" || strCommand == "start-disabled")
    {
        LOCK(pwalletMain->cs_wallet);
        EnsureWalletIsUnlocked();

        if((strCommand == "start-missing" || strCommand == "start-disabled") && !masternodeSync.IsMasternodeListSynced()) {
            throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, "You can't use this command until masternode list is synced");
        }

        int successful = 0;
        int failed = 0;

        UniValue resultsObj(UniValue::VOBJ);

        BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
            std::string errorMessage;

            CTxIn vin = CTxIn(uint256S(mne.getTxHash()), uint32_t(atoi(mne.getOutputIndex().c_str())));
            CMasternode *pmn = mnodeman.Find(vin);
            CMasternodeBroadcast mnb;

            if(strCommand == "start-missing" && pmn) continue;
            if(strCommand == "start-disabled" && pmn && pmn->IsEnabled()) continue;

            bool result = CMasternodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), errorMessage, mnb);

            UniValue statusObj(UniValue::VOBJ);
            statusObj.push_back(Pair("alias", mne.getAlias()));
            statusObj.push_back(Pair("result", result ? "successful" : "failed"));

            if(result) {
                successful++;
                mnodeman.UpdateMasternodeList(mnb);
                mnb.Relay();
            } else {
                failed++;
                statusObj.push_back(Pair("errorMessage", errorMessage));
            }

            resultsObj.push_back(Pair("status", statusObj));
        }

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
        std::vector<COutput> vPossibleCoins;
        pwalletMain->AvailableCoins(vPossibleCoins, true, NULL, false, ONLY_1000);

        UniValue obj(UniValue::VOBJ);
        BOOST_FOREACH(COutput& out, vPossibleCoins)
            obj.push_back(Pair(out.tx->GetHash().ToString(), strprintf("%d", out.i)));

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

    // 12.1 -- remove?
    // /*
    //     Shows which masternode wins by score each block
    // */
    // if (strCommand == "calcscore")
    // {

    //     int nHeight;
    //     {
    //         LOCK(cs_main);
    //         CBlockIndex* pindex = chainActive.Tip();
    //         if(!pindex) return NullUniValue;

    //         nHeight = pindex->nHeight;
    //     }

    //     int nLast = 10;

    //     if (params.size() >= 2){
    //         nLast = atoi(params[1].get_str());
    //     }
    //     UniValue obj(UniValue::VOBJ);

    //     std::vector<CMasternode> vMasternodes = mnodeman.GetFullMasternodeVector();
    //     for(int i = nHeight - nLast; i < nHeight + 20; i++){
    //         arith_uint256 nHigh = 0;
    //         CMasternode *pBestMasternode = NULL;
    //         BOOST_FOREACH(CMasternode& mn, vMasternodes) {
    //             arith_uint256 n = UintToArith256(mn.CalculateScore(1, i - 101));
    //             if(n > nHigh){
    //                 nHigh = n;
    //                 pBestMasternode = &mn;
    //             }
    //         }
    //         if(pBestMasternode)
    //             obj.push_back(Pair(strprintf("%d", i), pBestMasternode->vin.prevout.ToStringShort().c_str()));
    //     }

    //     return obj;
    // }

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

bool DecodeHexVecMnb(std::vector<CMasternodeBroadcast>& vecMnb, std::string strHexMnb) {

    if (!IsHex(strHexMnb))
        return false;

    vector<unsigned char> mnbData(ParseHex(strHexMnb));
    CDataStream ssData(mnbData, SER_NETWORK, PROTOCOL_VERSION);
    try {
        ssData >> vecMnb;
    }
    catch (const std::exception&) {
        return false;
    }

    return true;
}

UniValue masternodebroadcast(const UniValue& params, bool fHelp)
{
    string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();

    if (fHelp  ||
        (strCommand != "create-alias" && strCommand != "create-all" && strCommand != "decode" && strCommand != "relay"))
        throw runtime_error(
                "masternodebroadcast \"command\"... ( \"passphrase\" )\n"
                "Set of commands to create and relay masternode broadcast messages\n"
                "\nArguments:\n"
                "1. \"command\"        (string or set of strings, required) The command to execute\n"
                "2. \"passphrase\"     (string, optional) The wallet passphrase\n"
                "\nAvailable commands:\n"
                "  create-alias  - Create single remote masternode broadcast message by assigned alias configured in masternode.conf\n"
                "  create-all    - Create remote masternode broadcast messages for all masternodes configured in masternode.conf\n"
                "  decode        - Decode masternode broadcast message\n"
                "  relay         - Relay masternode broadcast message to the network\n"
                + HelpRequiringPassphrase());

    if (strCommand == "create-alias")
    {
        // wait for reindex and/or import to finish
        if (fImporting || fReindex)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Wait for reindex and/or import to finish");

        if (params.size() < 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Command needs at least 2 parameters");

        std::string alias = params[1].get_str();


        LOCK(pwalletMain->cs_wallet);
        EnsureWalletIsUnlocked();

        bool found = false;

        UniValue statusObj(UniValue::VOBJ);
        std::vector<CMasternodeBroadcast> vecMnb;

        statusObj.push_back(Pair("alias", alias));

        BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
            if(mne.getAlias() == alias) {
                found = true;
                std::string errorMessage;
                CMasternodeBroadcast mnb;

                bool result = CMasternodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), errorMessage, mnb, true);

                statusObj.push_back(Pair("result", result ? "successful" : "failed"));
                if(result) {
                    vecMnb.push_back(mnb);
                    CDataStream ssVecMnb(SER_NETWORK, PROTOCOL_VERSION);
                    ssVecMnb << vecMnb;
                    statusObj.push_back(Pair("hex", HexStr(ssVecMnb.begin(), ssVecMnb.end())));
                } else {
                    statusObj.push_back(Pair("errorMessage", errorMessage));
                }
                break;
            }
        }

        if(!found) {
            statusObj.push_back(Pair("result", "not found"));
            statusObj.push_back(Pair("errorMessage", "Could not find alias in config. Verify with list-conf."));
        }

        return statusObj;

    }

    if (strCommand == "create-all")
    {
        // wait for reindex and/or import to finish
        if (fImporting || fReindex)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Wait for reindex and/or import to finish");

        LOCK(pwalletMain->cs_wallet);
        EnsureWalletIsUnlocked();

        std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        int successful = 0;
        int failed = 0;

        UniValue resultsObj(UniValue::VOBJ);
        std::vector<CMasternodeBroadcast> vecMnb;

        BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
            std::string errorMessage;

            CTxIn vin = CTxIn(uint256S(mne.getTxHash()), uint32_t(atoi(mne.getOutputIndex().c_str())));
            CMasternodeBroadcast mnb;

            bool result = CMasternodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), errorMessage, mnb, true);

            UniValue statusObj(UniValue::VOBJ);
            statusObj.push_back(Pair("alias", mne.getAlias()));
            statusObj.push_back(Pair("result", result ? "successful" : "failed"));

            if(result) {
                successful++;
                vecMnb.push_back(mnb);
            } else {
                failed++;
                statusObj.push_back(Pair("errorMessage", errorMessage));
            }

            resultsObj.push_back(Pair("status", statusObj));
        }

        CDataStream ssVecMnb(SER_NETWORK, PROTOCOL_VERSION);
        ssVecMnb << vecMnb;
        UniValue returnObj(UniValue::VOBJ);
        returnObj.push_back(Pair("overall", strprintf("Successfully created broadcast messages for %d masternodes, failed to create %d, total %d", successful, failed, successful + failed)));
        returnObj.push_back(Pair("detail", resultsObj));
        returnObj.push_back(Pair("hex", HexStr(ssVecMnb.begin(), ssVecMnb.end())));

        return returnObj;
    }

    if (strCommand == "decode")
    {
        if (params.size() != 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'masternodebroadcast decode \"hexstring\"'");

        int successful = 0;
        int failed = 0;
        int nDos = 0;

        std::vector<CMasternodeBroadcast> vecMnb;
        UniValue returnObj(UniValue::VOBJ);

        if (!DecodeHexVecMnb(vecMnb, params[1].get_str()))
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Masternode broadcast message decode failed");

        BOOST_FOREACH(CMasternodeBroadcast& mnb, vecMnb) {
            UniValue resultObj(UniValue::VOBJ);

            if(mnb.VerifySignature(nDos)) {
                successful++;
                resultObj.push_back(Pair("vin", mnb.vin.ToString()));
                resultObj.push_back(Pair("addr", mnb.addr.ToString()));
                resultObj.push_back(Pair("pubkey", CBitcoinAddress(mnb.pubkey.GetID()).ToString()));
                resultObj.push_back(Pair("pubkey2", CBitcoinAddress(mnb.pubkey2.GetID()).ToString()));
                resultObj.push_back(Pair("vchSig", EncodeBase64(&mnb.vchSig[0], mnb.vchSig.size())));
                resultObj.push_back(Pair("sigTime", mnb.sigTime));
                resultObj.push_back(Pair("protocolVersion", mnb.protocolVersion));
                resultObj.push_back(Pair("nLastDsq", mnb.nLastDsq));

                UniValue lastPingObj(UniValue::VOBJ);
                lastPingObj.push_back(Pair("vin", mnb.lastPing.vin.ToString()));
                lastPingObj.push_back(Pair("blockHash", mnb.lastPing.blockHash.ToString()));
                lastPingObj.push_back(Pair("sigTime", mnb.lastPing.sigTime));
                lastPingObj.push_back(Pair("vchSig", EncodeBase64(&mnb.lastPing.vchSig[0], mnb.lastPing.vchSig.size())));

                resultObj.push_back(Pair("lastPing", lastPingObj));
            } else {
                failed++;
                resultObj.push_back(Pair("errorMessage", "Masternode broadcast signature verification failed"));
            }

            returnObj.push_back(Pair(mnb.GetHash().ToString(), resultObj));
        }

        returnObj.push_back(Pair("overall", strprintf("Successfully decoded broadcast messages for %d masternodes, failed to decode %d, total %d", successful, failed, successful + failed)));

        return returnObj;
    }

    if (strCommand == "relay")
    {
        if (params.size() < 2 || params.size() > 3)
            throw JSONRPCError(RPC_INVALID_PARAMETER,   "masternodebroadcast relay \"hexstring\" ( fast )\n"
                                                        "\nArguments:\n"
                                                        "1. \"hex\"      (string, required) Broadcast messages hex string\n"
                                                        "2. fast       (string, optional) If none, using safe method\n");

        int successful = 0;
        int failed = 0;
        bool fSafe = params.size() == 2;

        std::vector<CMasternodeBroadcast> vecMnb;
        UniValue returnObj(UniValue::VOBJ);

        if (!DecodeHexVecMnb(vecMnb, params[1].get_str()))
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Masternode broadcast message decode failed");

        // verify all signatures first, bailout if any of them broken
        BOOST_FOREACH(CMasternodeBroadcast& mnb, vecMnb) {
            UniValue resultObj(UniValue::VOBJ);

            resultObj.push_back(Pair("vin", mnb.vin.ToString()));
            resultObj.push_back(Pair("addr", mnb.addr.ToString()));

            int nDos = 0;
            bool fResult;
            if (mnb.VerifySignature(nDos)) {
                if (fSafe) {
                    fResult = mnodeman.CheckMnbAndUpdateMasternodeList(mnb, nDos);
                } else {
                    mnodeman.UpdateMasternodeList(mnb);
                    mnb.Relay();
                    fResult = true;
                }
            } else fResult = false;

            if(fResult) {
                successful++;
                resultObj.push_back(Pair(mnb.GetHash().ToString(), "successful"));
            } else {
                failed++;
                resultObj.push_back(Pair("errorMessage", "Masternode broadcast signature verification failed"));
            }

            returnObj.push_back(Pair(mnb.GetHash().ToString(), resultObj));
        }

        returnObj.push_back(Pair("overall", strprintf("Successfully relayed broadcast messages for %d masternodes, failed to relay %d, total %d", successful, failed, successful + failed)));

        return returnObj;
    }

    return NullUniValue;
}
