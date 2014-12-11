// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "core.h"
#include "db.h"
#include "init.h"
#include "masternode.h"
#include "activemasternode.h"
#include "rpcserver.h"
#include <boost/lexical_cast.hpp>

#include <fstream>
using namespace json_spirit;
using namespace std;



Value darksend(const Array& params, bool fHelp)
{
    if (fHelp || params.size() == 0)
        throw runtime_error(
            "darksend <darkcoinaddress> <amount>\n"
            "darkcoinaddress, reset, or auto (AutoDenominate)"
            "<amount> is a real and is rounded to the nearest 0.00000001"
            + HelpRequiringPassphrase());

    if(fMasterNode)
        return "DarkSend is not supported from masternodes";

    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");

    if(params[0].get_str() == "auto"){
        darkSendPool.DoAutomaticDenominating();
        return "DoAutomaticDenominating";
    }

    if(params[0].get_str() == "reset"){
        darkSendPool.SetNull(true);
        darkSendPool.UnlockCoins();
        return "successfully reset darksend";
    }

    if (params.size() != 2)
        throw runtime_error(
            "darksend <darkcoinaddress> <amount>\n"
            "darkcoinaddress, denominate, or auto (AutoDenominate)"
            "<amount> is a real and is rounded to the nearest 0.00000001"
            + HelpRequiringPassphrase());

    CBitcoinAddress address(params[0].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid DarkCoin address");

    // Amount
    int64_t nAmount = AmountFromValue(params[1]);

    // Wallet comments
    CWalletTx wtx;
    string strError = pwalletMain->SendMoneyToDestination(address.Get(), nAmount, wtx, ONLY_DENOMINATED);
    if (strError != "")
        throw JSONRPCError(RPC_WALLET_ERROR, strError);

    return wtx.GetHash().GetHex();
}


Value getpoolinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getpoolinfo\n"
            "Returns an object containing anonymous pool-related information.");

    Object obj;
    obj.push_back(Pair("current_masternode",        GetCurrentMasterNode()));
    obj.push_back(Pair("state",        darkSendPool.GetState()));
    obj.push_back(Pair("entries",      darkSendPool.GetEntriesCount()));
    obj.push_back(Pair("entries_accepted",      darkSendPool.GetCountEntriesAccepted()));
    return obj;
}

Value masternode(const Array& params, bool fHelp)
{
    string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();

    if (fHelp  ||
        (strCommand != "start" && strCommand != "start-many" && strCommand != "stop" && strCommand != "list" && strCommand != "count"  && strCommand != "enforce"
            && strCommand != "debug" && strCommand != "current" && strCommand != "winners" && strCommand != "genkey" && strCommand != "connect"))
        throw runtime_error(
            "masternode <start|start-many|stop|list|count|debug|current|winners|genkey|enforce> passphrase\n");

    if (strCommand == "stop")
    {
        if(!fMasterNode) return "you must set masternode=1 in the configuration";

        if(pwalletMain->IsLocked()) {
            SecureString strWalletPass;
            strWalletPass.reserve(100);

            if (params.size() == 2){
                strWalletPass = params[1].get_str().c_str();
            } else {
                throw runtime_error(
                    "Your wallet is locked, passphrase is required\n");
            }

            if(!pwalletMain->Unlock(strWalletPass)){
                return "incorrect passphrase";
            }
        }

        activeMasternode.RegisterAsMasterNode(true);
        pwalletMain->Lock();

        if(activeMasternode.isCapableMasterNode == MASTERNODE_STOPPED) return "successfully stopped masternode";
        if(activeMasternode.isCapableMasterNode == MASTERNODE_NOT_CAPABLE) return "not capable masternode";

        return "unknown";
    }

    if (strCommand == "list")
    {
        std::string strCommand = "active";

        if (params.size() == 2){
            strCommand = params[1].get_str().c_str();
        }

        if (strCommand != "active" && strCommand != "vin" && strCommand != "pubkey" && strCommand != "lastseen" && strCommand != "activeseconds" && strCommand != "rank" && strCommand != "protocol"){
            throw runtime_error(
                "list supports 'active', 'vin', 'pubkey', 'lastseen', 'activeseconds', 'rank', 'protocol'\n");
        }

        Object obj;
        BOOST_FOREACH(CMasterNode mn, darkSendMasterNodes) {
            mn.Check();

            if(strCommand == "active"){
                obj.push_back(Pair(mn.addr.ToString().c_str(),       (int)mn.IsEnabled()));
            } else if (strCommand == "vin") {
                obj.push_back(Pair(mn.addr.ToString().c_str(),       mn.vin.prevout.hash.ToString().c_str()));
            } else if (strCommand == "pubkey") {
                CScript pubkey;
                pubkey.SetDestination(mn.pubkey.GetID());
                CTxDestination address1;
                ExtractDestination(pubkey, address1);
                CBitcoinAddress address2(address1);

                obj.push_back(Pair(mn.addr.ToString().c_str(),       address2.ToString().c_str()));
            } else if (strCommand == "protocol") {
                obj.push_back(Pair(mn.addr.ToString().c_str(),       (int64_t)mn.protocolVersion));
            } else if (strCommand == "lastseen") {
                obj.push_back(Pair(mn.addr.ToString().c_str(),       (int64_t)mn.lastTimeSeen));
            } else if (strCommand == "activeseconds") {
                obj.push_back(Pair(mn.addr.ToString().c_str(),       (int64_t)(mn.lastTimeSeen - mn.now)));
            } else if (strCommand == "rank") {
                obj.push_back(Pair(mn.addr.ToString().c_str(),       (int)(GetMasternodeRank(mn.vin, chainActive.Tip()->nHeight))));
            }
        }
        return obj;
    }
    if (strCommand == "count") return (int)darkSendMasterNodes.size();

    if (strCommand == "start")
    {
        if(!fMasterNode) return "you must set masternode=1 in the configuration";

        if(pwalletMain->IsLocked()) {
            SecureString strWalletPass;
            strWalletPass.reserve(100);

            if (params.size() == 2){
                strWalletPass = params[1].get_str().c_str();
            } else {
                throw runtime_error(
                    "Your wallet is locked, passphrase is required\n");
            }

            if(!pwalletMain->Unlock(strWalletPass)){
                return "incorrect passphrase";
            }
        }

        activeMasternode.RegisterAsMasterNode(false);
        pwalletMain->Lock();

        if(activeMasternode.isCapableMasterNode == MASTERNODE_REMOTELY_ENABLED) return "masternode started remotely";
        if(activeMasternode.isCapableMasterNode == MASTERNODE_INPUT_TOO_NEW) return "masternode input must have at least 15 confirmations";
        if(activeMasternode.isCapableMasterNode == MASTERNODE_STOPPED) return "masternode is stopped";
        if(activeMasternode.isCapableMasterNode == MASTERNODE_IS_CAPABLE) return "successfully started masternode";
        if(activeMasternode.masternodePortOpen == MASTERNODE_PORT_NOT_OPEN) return "inbound port is not open. Please open it and try again. (19999 for testnet and 9999 for mainnet)";
        if(activeMasternode.isCapableMasterNode == MASTERNODE_NOT_CAPABLE) return "not capable masternode";
        if(activeMasternode.isCapableMasterNode == MASTERNODE_SYNC_IN_PROCESS) return "sync in process. Must wait until client is synced to start.";

        return "unknown";
    }

    if (strCommand == "start-many")
    {
        boost::filesystem::path pathDebug = GetDataDir() / "masternode.conf";
        std::ifstream infile(pathDebug.string().c_str());

        std::string line;
        int total = 0;
        int successful = 0;
        int fail = 0;
        while (std::getline(infile, line))
        {
            std::istringstream iss(line);
            std::string a, b;
            if (!(iss >> a >> b)) { break; } // error

            total++;
            if(activeMasternode.RegisterAsMasterNodeRemoteOnly(a, b)){
                successful++;
            } else {
                fail++;
            }
        }

        printf(" Successfully started %d masternodes, failed to start %d, total %d\n", successful, fail, total);
        return "";

    }

    if (strCommand == "debug")
    {
        if(activeMasternode.isCapableMasterNode == MASTERNODE_REMOTELY_ENABLED) return "masternode started remotely";
        if(activeMasternode.isCapableMasterNode == MASTERNODE_INPUT_TOO_NEW) return "masternode input must have at least 15 confirmations";
        if(activeMasternode.isCapableMasterNode == MASTERNODE_IS_CAPABLE) return "successfully started masternode";
        if(activeMasternode.isCapableMasterNode == MASTERNODE_STOPPED) return "masternode is stopped";
        if(activeMasternode.masternodePortOpen == MASTERNODE_PORT_NOT_OPEN) return "inbound port is not open. Please open it and try again. (19999 for testnet and 9999 for mainnet)";
        if(activeMasternode.isCapableMasterNode == MASTERNODE_NOT_CAPABLE) return "not capable masternode";
        if(activeMasternode.isCapableMasterNode == MASTERNODE_SYNC_IN_PROCESS) return "sync in process. Must wait until client is synced to start.";

        CTxIn vin = CTxIn();
        CPubKey pubkey = CScript();
        CKey key;
        bool found = activeMasternode.GetMasterNodeVin(vin, pubkey, key);
        if(!found){
            return "Missing masternode input, please look at the documentation for instructions on masternode creation";
        } else {
            return "No problems were found";
        }
    }

    if (strCommand == "create")
    {

        return "Not implemented yet, please look at the documentation for instructions on masternode creation";
    }

    if (strCommand == "current")
    {
        int winner = GetCurrentMasterNode(1);
        if(winner >= 0) {
            return darkSendMasterNodes[winner].addr.ToString().c_str();
        }

        return "unknown";
    }

    if (strCommand == "genkey")
    {
        CKey secret;
        secret.MakeNewKey(false);

        return CBitcoinSecret(secret).ToString();
    }

    if (strCommand == "winners")
    {
        Object obj;

        for(int nHeight = chainActive.Tip()->nHeight-10; nHeight < chainActive.Tip()->nHeight+20; nHeight++)
        {
            CScript payee;
            if(masternodePayments.GetBlockPayee(nHeight, payee)){
                CTxDestination address1;
                ExtractDestination(payee, address1);
                CBitcoinAddress address2(address1);
                obj.push_back(Pair(boost::lexical_cast<std::string>(nHeight),       address2.ToString().c_str()));
            } else {
                obj.push_back(Pair(boost::lexical_cast<std::string>(nHeight),       ""));
            }
        }

        return obj;
    }

    if(strCommand == "enforce")
    {
        return (uint64_t)enforceMasternodePaymentsTime;
    }

    if(strCommand == "connect")
    {
        std::string strAddress = "";
        if (params.size() == 2){
            strAddress = params[1].get_str().c_str();
        } else {
            throw runtime_error(
                "Masternode address required\n");
        }

        CService addr = CService(strAddress);

        if(ConnectNode((CAddress)addr, NULL, true)){
            return "successfully connected";
        } else {
            return "error connecting";
        }
    }

    return Value::null;
}

