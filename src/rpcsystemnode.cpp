// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "db.h"
#include "init.h"
#include "systemnodeconfig.h"
#include "systemnode.h"
#include "systemnodeman.h"
#include "activesystemnode.h"
#include "rpcserver.h"
#include "utilmoneystr.h"
#include "wallet.h"
#include "key.h"
#include "base58.h"

#include <fstream>
#include <string>
using namespace std;
using namespace json_spirit;

Value sngetpoolinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getpoolinfo\n"
            "Returns an object containing systemnode pool-related information.");

    Object obj;
    obj.push_back(Pair("current_systemnode",        snodeman.GetCurrentSystemNode()->addr.ToString()));
    return obj;
}

Value systemnode(const Array& params, bool fHelp)
{
    string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();

    if (fHelp  ||
        (strCommand != "start" && strCommand != "start-alias" && strCommand != "start-many" && strCommand != "start-all" && 
         strCommand != "start-missing" && strCommand != "start-disabled" && strCommand != "list" && strCommand != "list-conf" && 
         strCommand != "count"  && strCommand != "enforce" && strCommand != "debug" && strCommand != "current" && 
         strCommand != "winners" && strCommand != "connect" && strCommand != "outputs" && 
         strCommand != "status" && strCommand != "calcscore"))
        throw runtime_error(
                "systemnode \"command\"... ( \"passphrase\" )\n"
                "Set of commands to execute systemnode related actions\n"
                "\nArguments:\n"
                "1. \"command\"        (string or set of strings, required) The command to execute\n"
                "2. \"passphrase\"     (string, optional) The wallet passphrase\n"
                "\nAvailable commands:\n"
                "  count        - Print number of all known systemnodes (optional: 'ds', 'enabled', 'all', 'qualify')\n"
                "  current      - Print info on current systemnode winner\n"
                "  debug        - Print systemnode status\n"
                "  enforce      - Enforce systemnode payments\n"
                "  outputs      - Print systemnode compatible outputs\n"
                "  start        - Start systemnode configured in crown.conf\n"
                "  start-alias  - Start single systemnode by assigned alias configured in systemnode.conf\n"
                "  start-<mode> - Start systemnodes configured in systemnode.conf (<mode>: 'all', 'missing', 'disabled')\n"
                "  status       - Print systemnode status information\n"
                "  list         - Print list of all known systemnodes (see systemnodelist for more info)\n"
                "  list-conf    - Print systemnode.conf in JSON format\n"
                "  winners      - Print list of systemnode winners\n"
                );

    if (strCommand == "list")
    {
        Array newParams(params.size() - 1);
        std::copy(params.begin() + 1, params.end(), newParams.begin());
        return systemnodelist(newParams, fHelp);
    }

    if (strCommand == "budget")
    {
        return "Show budgets";
    }

    if(strCommand == "connect")
    {
        std::string strAddress = "";
        if (params.size() == 2) {
            strAddress = params[1].get_str();
        } else {
            throw runtime_error("Systemnode address required\n");
        }

        CService addr = CService(strAddress);

        CNode *pnode = ConnectNode((CAddress)addr, NULL, false);
        if(pnode){
            pnode->Release();
            return "successfully connected";
        } else {
            throw runtime_error("error connecting\n");
        }
    }

    if (strCommand == "count")
    {
        if (params.size() > 2){
            throw runtime_error("too many parameters\n");
        }
        if (params.size() == 2)
        {
            int nCount = 0;

            if(chainActive.Tip())
                snodeman.GetNextSystemnodeInQueueForPayment(chainActive.Tip()->nHeight, true, nCount);

            if(params[1] == "ls") return snodeman.CountEnabled(MIN_POOL_PEER_PROTO_VERSION);
            if(params[1] == "enabled") return snodeman.CountEnabled();
            if(params[1] == "qualify") return nCount;
            if(params[1] == "all") return strprintf("Total: %d (LS Compatible: %d / Enabled: %d / Qualify: %d)",
                                                    snodeman.size(),
                                                    snodeman.CountEnabled(MIN_POOL_PEER_PROTO_VERSION),
                                                    snodeman.CountEnabled(),
                                                    nCount);
        }
        return snodeman.size();
    }

    if (strCommand == "current")
    {
        CSystemnode* winner = snodeman.GetCurrentSystemNode(1);
        if(winner) {
            Object obj;

            obj.push_back(Pair("IP:port",       winner->addr.ToString()));
            obj.push_back(Pair("protocol",      (int64_t)winner->protocolVersion));
            obj.push_back(Pair("vin",           winner->vin.prevout.hash.ToString()));
            obj.push_back(Pair("pubkey",        CBitcoinAddress(winner->pubkey.GetID()).ToString()));
            obj.push_back(Pair("lastseen",      (winner->lastPing == CSystemnodePing()) ? winner->sigTime :
                                                        (int64_t)winner->lastPing.sigTime));
            obj.push_back(Pair("activeseconds", (winner->lastPing == CSystemnodePing()) ? 0 :
                                                        (int64_t)(winner->lastPing.sigTime - winner->sigTime)));
            return obj;
        }

        return "unknown";
    }

    if (strCommand == "debug")
    {
        if(activeSystemnode.status != ACTIVE_SYSTEMNODE_INITIAL || !systemnodeSync.IsSynced())
            return activeSystemnode.GetStatus();

        CTxIn vin = CTxIn();
        CPubKey pubkey;
        CKey key;
        if(!pwalletMain || !pwalletMain->GetSystemnodeVinAndKeys(vin, pubkey, key))
            throw runtime_error("Missing systemnode input, please look at the documentation for instructions on systemnode creation\n");
        return activeSystemnode.GetStatus();
    }

    if(strCommand == "enforce")
    {
        return (uint64_t)enforceSystemnodePaymentsTime;
    }

    if (strCommand == "start")
    {
        if(!fSystemNode) throw runtime_error("you must set systemnode=1 in the configuration\n");

        {
            LOCK(pwalletMain->cs_wallet);
            EnsureWalletIsUnlocked();
        }

        if(activeSystemnode.status != ACTIVE_SYSTEMNODE_STARTED){
            activeSystemnode.status = ACTIVE_SYSTEMNODE_INITIAL; // TODO: consider better way
            activeSystemnode.ManageStatus();
        }

        return activeSystemnode.GetStatus();
    }

    if (strCommand == "start-alias")
    {
        if (params.size() < 2){
            throw runtime_error("command needs at least 2 parameters\n");
        }

        {
            LOCK(pwalletMain->cs_wallet);
            EnsureWalletIsUnlocked();
        }

        std::string alias = params[1].get_str();

        bool found = false;

        Object statusObj;
        statusObj.push_back(Pair("alias", alias));

        BOOST_FOREACH(CNodeEntry mne, systemnodeConfig.getEntries()) {
            if(mne.getAlias() == alias) {
                found = true;
                std::string errorMessage;
                CSystemnodeBroadcast snb;

                bool result = CSystemnodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), errorMessage, snb);

                statusObj.push_back(Pair("result", result ? "successful" : "failed"));
                if(result) {
                    snodeman.UpdateSystemnodeList(snb);
                    snb.Relay();
                } else {
                    statusObj.push_back(Pair("errorMessage", errorMessage));
                }
                break;
            }
        }

        if(!found) {
            statusObj.push_back(Pair("result", "failed"));
            statusObj.push_back(Pair("errorMessage", "could not find alias in config. Verify with list-conf."));
        }

        return statusObj;

    }

    if (strCommand == "create")
    {

        throw runtime_error("Not implemented yet, please look at the documentation for instructions on systemnode creation\n");
    }

    if(strCommand == "list-conf")
    {
        std::vector<CNodeEntry> mnEntries;
        mnEntries = systemnodeConfig.getEntries();

        Object resultObj;

        BOOST_FOREACH(CNodeEntry mne, systemnodeConfig.getEntries()) {
            CTxIn vin = CTxIn(uint256S(mne.getTxHash()), uint32_t(atoi(mne.getOutputIndex().c_str())));
            CSystemnode *pmn = snodeman.Find(vin);

            std::string strStatus = pmn ? pmn->Status() : "MISSING";

            Object mnObj;
            mnObj.push_back(Pair("alias", mne.getAlias()));
            mnObj.push_back(Pair("address", mne.getIp()));
            mnObj.push_back(Pair("privateKey", mne.getPrivKey()));
            mnObj.push_back(Pair("txHash", mne.getTxHash()));
            mnObj.push_back(Pair("outputIndex", mne.getOutputIndex()));
            mnObj.push_back(Pair("status", strStatus));
            resultObj.push_back(Pair("systemnode", mnObj));
        }

        return resultObj;
    }

    if (strCommand == "outputs"){
        // Find possible candidates
        std::vector<COutput> vPossibleCoins;
        pwalletMain->AvailableCoins(vPossibleCoins, true, NULL, ONLY_500);

        Object obj;
        BOOST_FOREACH(COutput& out, vPossibleCoins)
            obj.push_back(Pair(out.tx->GetHash().ToString(), strprintf("%d", out.i)));

        return obj;
    }

    if (strCommand == "start-many" || strCommand == "start-all" || strCommand == "start-missing" || strCommand == "start-disabled")
    {
        {
            LOCK(pwalletMain->cs_wallet);
            EnsureWalletIsUnlocked();
        }

        if((strCommand == "start-missing" || strCommand == "start-disabled") &&
         (systemnodeSync.RequestedSystemnodeAssets <= SYSTEMNODE_SYNC_LIST ||
          systemnodeSync.RequestedSystemnodeAssets == SYSTEMNODE_SYNC_FAILED)) {
            throw runtime_error("You can't use this command until systemnode list is synced\n");
        }

        std::vector<CNodeEntry> mnEntries;
        mnEntries = systemnodeConfig.getEntries();

        int successful = 0;
        int failed = 0;

        Object resultsObj;

        BOOST_FOREACH(CNodeEntry mne, systemnodeConfig.getEntries()) {
            std::string errorMessage;

            CTxIn vin = CTxIn(uint256S(mne.getTxHash()), uint32_t(atoi(mne.getOutputIndex().c_str())));
            CSystemnode *pmn = snodeman.Find(vin);
            CSystemnodeBroadcast snb;

            if(strCommand == "start-missing" && pmn) continue;
            if(strCommand == "start-disabled" && pmn && pmn->IsEnabled()) continue;

            bool result = CSystemnodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), errorMessage, snb);

            Object statusObj;
            statusObj.push_back(Pair("alias", mne.getAlias()));
            statusObj.push_back(Pair("result", result ? "successful" : "failed"));

            if(result) {
                successful++;
                snodeman.UpdateSystemnodeList(snb);
                snb.Relay();
            } else {
                failed++;
                statusObj.push_back(Pair("errorMessage", errorMessage));
            }

            resultsObj.push_back(Pair("status", statusObj));
        }

        Object returnObj;
        returnObj.push_back(Pair("overall", strprintf("Successfully started %d systemnodes, failed to start %d, total %d", successful, failed, successful + failed)));
        returnObj.push_back(Pair("detail", resultsObj));

        return returnObj;
    }

    if(strCommand == "status")
    {
        if(!fSystemNode) throw runtime_error("This is not a systemnode\n");

        Object mnObj;
        CSystemnode *pmn = snodeman.Find(activeSystemnode.vin);

        mnObj.push_back(Pair("vin", activeSystemnode.vin.ToString()));
        mnObj.push_back(Pair("service", activeSystemnode.service.ToString()));
        if (pmn) mnObj.push_back(Pair("pubkey", CBitcoinAddress(pmn->pubkey.GetID()).ToString()));
        mnObj.push_back(Pair("status", activeSystemnode.GetStatus()));
        return mnObj;
    }

    if (strCommand == "winners")
    {
        int nLast = 10;

        if (params.size() >= 2){
            nLast = atoi(params[1].get_str());
        }

        Object obj;

        for(int nHeight = chainActive.Tip()->nHeight-nLast; nHeight < chainActive.Tip()->nHeight+20; nHeight++)
        {
            obj.push_back(Pair(strprintf("%d", nHeight), SNGetRequiredPaymentsString(nHeight)));
        }

        return obj;
    }

    /*
        Shows which systemnode wins by score each block
    */
    if (strCommand == "calcscore")
    {

        int nLast = 10;

        if (params.size() >= 2){
            nLast = atoi(params[1].get_str());
        }
        Object obj;

        std::vector<CSystemnode> vSystemnodes = snodeman.GetFullSystemnodeVector();
        for(int nHeight = chainActive.Tip()->nHeight-nLast; nHeight < chainActive.Tip()->nHeight+20; nHeight++){
            arith_uint256  nHigh = 0;
            CSystemnode *pBestSystemnode = NULL;
            BOOST_FOREACH(CSystemnode& mn, vSystemnodes) {
                arith_uint256  n = mn.CalculateScore(nHeight - 100);
                if(n > nHigh){
                    nHigh = n;
                    pBestSystemnode = &mn;
                }
            }
            if(pBestSystemnode)
                obj.push_back(Pair(strprintf("%d", nHeight), pBestSystemnode->vin.prevout.ToStringShort().c_str()));
        }

        return obj;
    }

    return Value::null;
}

Value systemnodelist(const Array& params, bool fHelp)
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
                "systemnodelist ( \"mode\" \"filter\" )\n"
                "Get a list of systemnodes in different modes\n"
                "\nArguments:\n"
                "1. \"mode\"      (string, optional/required to use filter, defaults = status) The mode to run list in\n"
                "2. \"filter\"    (string, optional) Filter results. Partial match by IP by default in all modes,\n"
                "                                    additional matches in some modes are also available\n"
                "\nAvailable modes:\n"
                "  activeseconds  - Print number of seconds systemnode recognized by the network as enabled\n"
                "                   (since latest issued \"systemnode start/start-many/start-alias\")\n"
                "  addr           - Print ip address associated with a systemnode (can be additionally filtered, partial match)\n"
                "  full           - Print info in format 'status protocol pubkey IP lastseen activeseconds lastpaid'\n"
                "                   (can be additionally filtered, partial match)\n"
                "  lastseen       - Print timestamp of when a systemnode was last seen on the network\n"
                "  lastpaid       - The last time a node was paid on the network\n"
                "  protocol       - Print protocol of a systemnode (can be additionally filtered, exact match))\n"
                "  pubkey         - Print public key associated with a systemnode (can be additionally filtered,\n"
                "                   partial match)\n"
                "  rank           - Print rank of a systemnode based on current block\n"
                "  status         - Print systemnode status: ENABLED / EXPIRED / VIN_SPENT / REMOVE / POS_ERROR\n"
                "                   (can be additionally filtered, partial match)\n"
                );
    }

    Object obj;
    if (strMode == "rank") {
        std::vector<pair<int, CSystemnode> > vSystemnodeRanks = snodeman.GetSystemnodeRanks(chainActive.Tip()->nHeight);
        BOOST_FOREACH(PAIRTYPE(int, CSystemnode)& s, vSystemnodeRanks) {
            std::string strVin = s.second.vin.prevout.ToStringShort();
            if(strFilter !="" && strVin.find(strFilter) == string::npos) continue;
            obj.push_back(Pair(strVin,       s.first));
        }
    } else {
        std::vector<CSystemnode> vSystemnodes = snodeman.GetFullSystemnodeVector();
        BOOST_FOREACH(CSystemnode& mn, vSystemnodes) {
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

bool DecodeHexVecSnb(std::vector<CSystemnodeBroadcast>& vecSnb, std::string strHexSnb) {

    if (!IsHex(strHexSnb))
        return false;

    vector<unsigned char> snbData(ParseHex(strHexSnb));
    CDataStream ssData(snbData, SER_NETWORK, PROTOCOL_VERSION);
    try {
        ssData >> vecSnb;
    }
    catch (const std::exception&) {
        return false;
    }

    return true;
}

Value systemnodebroadcast(const Array& params, bool fHelp)
{
    string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();

    if (fHelp  ||
        (strCommand != "create-alias" && strCommand != "create-all" && strCommand != "decode" && strCommand != "relay"))
        throw runtime_error(
                "systemnodebroadcast \"command\"... ( \"passphrase\" )\n"
                "Set of commands to create and relay systemnode broadcast messages\n"
                "\nArguments:\n"
                "1. \"command\"        (string or set of strings, required) The command to execute\n"
                "2. \"passphrase\"     (string, optional) The wallet passphrase\n"
                "\nAvailable commands:\n"
                "  create-alias  - Create single remote systemnode broadcast message by assigned alias configured in systemnode.conf\n"
                "  create-all    - Create remote systemnode broadcast messages for all systemnodes configured in systemnode.conf\n"
                "  decode        - Decode systemnode broadcast message\n"
                "  relay         - Relay systemnode broadcast message to the network\n"
                + HelpRequiringPassphrase());

    if (strCommand == "create-alias")
    {
        // wait for reindex and/or import to finish
        if (fImporting || fReindex)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Wait for reindex and/or import to finish");

        if (params.size() < 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Command needs at least 2 parameters");

        {
            LOCK(pwalletMain->cs_wallet);
            EnsureWalletIsUnlocked();
        }

        std::string alias = params[1].get_str();

        bool found = false;

        Object statusObj;
        std::vector<CSystemnodeBroadcast> vecMnb;

        statusObj.push_back(Pair("alias", alias));

        BOOST_FOREACH(CNodeEntry mne, systemnodeConfig.getEntries()) {
            if(mne.getAlias() == alias) {
                found = true;
                std::string errorMessage;
                CSystemnodeBroadcast snb;

                bool result = CSystemnodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), errorMessage, snb, true);

                statusObj.push_back(Pair("result", result ? "successful" : "failed"));
                if(result) {
                    vecMnb.push_back(snb);
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

        {
            LOCK(pwalletMain->cs_wallet);
            EnsureWalletIsUnlocked();
        }

        std::vector<CNodeEntry> mnEntries;
        mnEntries = systemnodeConfig.getEntries();

        int successful = 0;
        int failed = 0;

        Object resultsObj;
        std::vector<CSystemnodeBroadcast> vecMnb;

        BOOST_FOREACH(CNodeEntry mne, systemnodeConfig.getEntries()) {
            std::string errorMessage;

            CTxIn vin = CTxIn(uint256S(mne.getTxHash()), uint32_t(atoi(mne.getOutputIndex().c_str())));
            CSystemnodeBroadcast snb;

            bool result = CSystemnodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), errorMessage, snb, true);

            Object statusObj;
            statusObj.push_back(Pair("alias", mne.getAlias()));
            statusObj.push_back(Pair("result", result ? "successful" : "failed"));

            if(result) {
                successful++;
                vecMnb.push_back(snb);
            } else {
                failed++;
                statusObj.push_back(Pair("errorMessage", errorMessage));
            }

            resultsObj.push_back(Pair("status", statusObj));
        }

        CDataStream ssVecMnb(SER_NETWORK, PROTOCOL_VERSION);
        ssVecMnb << vecMnb;
        Object returnObj;
        returnObj.push_back(Pair("overall", strprintf("Successfully created broadcast messages for %d systemnodes, failed to create %d, total %d", successful, failed, successful + failed)));
        returnObj.push_back(Pair("detail", resultsObj));
        returnObj.push_back(Pair("hex", HexStr(ssVecMnb.begin(), ssVecMnb.end())));

        return returnObj;
    }

    if (strCommand == "decode")
    {
        if (params.size() != 2)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Correct usage is 'systemnodebroadcast decode \"hexstring\"'");

        int successful = 0;
        int failed = 0;

        std::vector<CSystemnodeBroadcast> vecMnb;
        Object returnObj;

        if (!DecodeHexVecSnb(vecMnb, params[1].get_str()))
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Systemnode broadcast message decode failed");

        BOOST_FOREACH(CSystemnodeBroadcast& snb, vecMnb) {
            Object resultObj;

            if(snb.VerifySignature()) {
                successful++;
                resultObj.push_back(Pair("vin", snb.vin.ToString()));
                resultObj.push_back(Pair("addr", snb.addr.ToString()));
                resultObj.push_back(Pair("pubkey", CBitcoinAddress(snb.pubkey.GetID()).ToString()));
                resultObj.push_back(Pair("pubkey2", CBitcoinAddress(snb.pubkey2.GetID()).ToString()));
                resultObj.push_back(Pair("vchSig", EncodeBase64(&snb.sig[0], snb.sig.size())));
                resultObj.push_back(Pair("sigTime", snb.sigTime));
                resultObj.push_back(Pair("protocolVersion", snb.protocolVersion));

                Object lastPingObj;
                lastPingObj.push_back(Pair("vin", snb.lastPing.vin.ToString()));
                lastPingObj.push_back(Pair("blockHash", snb.lastPing.blockHash.ToString()));
                lastPingObj.push_back(Pair("sigTime", snb.lastPing.sigTime));
                lastPingObj.push_back(Pair("vchSig", EncodeBase64(&snb.lastPing.vchSig[0], snb.lastPing.vchSig.size())));

                resultObj.push_back(Pair("lastPing", lastPingObj));
            } else {
                failed++;
                resultObj.push_back(Pair("errorMessage", "Systemnode broadcast signature verification failed"));
            }

            returnObj.push_back(Pair(snb.GetHash().ToString(), resultObj));
        }

        returnObj.push_back(Pair("overall", strprintf("Successfully decoded broadcast messages for %d systemnodes, failed to decode %d, total %d", successful, failed, successful + failed)));

        return returnObj;
    }

    if (strCommand == "relay")
    {
        if (params.size() < 2 || params.size() > 3)
            throw JSONRPCError(RPC_INVALID_PARAMETER,   "systemnodebroadcast relay \"hexstring\" ( fast )\n"
                                                        "\nArguments:\n"
                                                        "1. \"hex\"      (string, required) Broadcast messages hex string\n"
                                                        "2. fast       (string, optional) If none, using safe method\n");

        int successful = 0;
        int failed = 0;
        bool fSafe = params.size() == 2;

        std::vector<CSystemnodeBroadcast> vecMnb;
        Object returnObj;

        if (!DecodeHexVecSnb(vecMnb, params[1].get_str()))
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Systemnode broadcast message decode failed");

        // verify all signatures first, bailout if any of them broken
        BOOST_FOREACH(CSystemnodeBroadcast& snb, vecMnb) {
            Object resultObj;

            resultObj.push_back(Pair("vin", snb.vin.ToString()));
            resultObj.push_back(Pair("addr", snb.addr.ToString()));

            int nDos = 0;
            bool fResult;
            if (snb.VerifySignature()) {
                if (fSafe) {
                    fResult = snodeman.CheckSnbAndUpdateSystemnodeList(snb, nDos);
                } else {
                    snodeman.UpdateSystemnodeList(snb);
                    snb.Relay();
                    fResult = true;
                }
            } else fResult = false;

            if(fResult) {
                successful++;
                snodeman.UpdateSystemnodeList(snb);
                snb.Relay();
                resultObj.push_back(Pair(snb.GetHash().ToString(), "successful"));
            } else {
                failed++;
                resultObj.push_back(Pair("errorMessage", "Systemnode broadcast signature verification failed"));
            }

            returnObj.push_back(Pair(snb.GetHash().ToString(), resultObj));
        }

        returnObj.push_back(Pair("overall", strprintf("Successfully relayed broadcast messages for %d systemnodes, failed to relay %d, total %d", successful, failed, successful + failed)));

        return returnObj;
    }

    return Value::null;
}
