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
#include "rpcserver.h"
#include "utilmoneystr.h"
#include "wallet.h"
#include "key.h"
#include "base58.h"

#include <fstream>
#include <string>
using namespace std;
using namespace json_spirit;

Value systemnode(const Array& params, bool fHelp)
{
    string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();

    if (fHelp  ||
        (strCommand != "start" && strCommand != "start-alias" && strCommand != "start-many" && strCommand != "start-all" && 
         strCommand != "start-missing" && strCommand != "start-disabled" && strCommand != "list" && strCommand != "list-conf" && 
         strCommand != "count"  && strCommand != "enforce" && strCommand != "debug" && strCommand != "current" && 
         strCommand != "winners" && strCommand != "genkey" && strCommand != "connect" && strCommand != "outputs" && 
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
                "  genkey       - Generate new systemnodeprivkey\n"
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
        // TODO to be implemented
        return "Option is not implemented yet";
    }

    if (strCommand == "current")
    {
        // TODO to be implemented
        return "Option is not implemented yet";
    }

    if (strCommand == "genkey")
    {
        CKey secret;
        secret.MakeNewKey(false);

        return CBitcoinSecret(secret).ToString();
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

        //if((strCommand == "start-missing" || strCommand == "start-disabled") &&
        // (systemnodeSync.RequestedSystemnodeAssets <= THRONE_SYNC_LIST ||
        //  systemnodeSync.RequestedSystemnodeAssets == THRONE_SYNC_FAILED)) {
        //    throw runtime_error("You can't use this command until systemnode list is synced\n");
        //}

        std::vector<CSystemnodeConfig::CSystemnodeEntry> mnEntries;
        mnEntries = systemnodeConfig.getEntries();

        int successful = 0;
        int failed = 0;

        Object resultsObj;

        BOOST_FOREACH(CSystemnodeConfig::CSystemnodeEntry mne, systemnodeConfig.getEntries()) {
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

    return Value::null;
}

Value systemnodelist(const Array& params, bool fHelp)
{
    return Value::null;
}

Value systemnodebroadcast(const Array& params, bool fHelp)
{
    return Value::null;
}
