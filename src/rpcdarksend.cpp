// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "core.h"
#include "db.h"
#include "init.h"
#include "activemasternode.h"
#include "masternodeman.h"
#include "masternodeconfig.h"
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

    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");

    if(params[0].get_str() == "auto"){
        if(fMasterNode)
            return "DarkSend is not supported from masternodes";

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
    obj.push_back(Pair("current_masternode",        mnodeman.GetCurrentMasterNode()->addr.ToString()));
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
        (strCommand != "start" && strCommand != "start-alias" && strCommand != "start-many" && strCommand != "stop" && strCommand != "stop-alias" && strCommand != "stop-many" && strCommand != "list" && strCommand != "list-conf" && strCommand != "count"  && strCommand != "enforce"
            && strCommand != "debug" && strCommand != "current" && strCommand != "winners" && strCommand != "genkey" && strCommand != "connect" && strCommand != "outputs"))
        throw runtime_error(
                "masternode \"command\"... ( \"passphrase\" )\n"
                "Set of commands to execute masternode related actions\n"
                "\nArguments:\n"
                "1. \"command\"        (string or set of strings, required) The command to execute\n"
                "2. \"passphrase\"     (string, optional) The wallet passphrase\n"
                "\nAvailable commands:\n"
                "  count        - Print number of all known masternodes (optional: 'enabled', 'both')\n"
                "  current      - Print info on current masternode winner\n"
                "  debug        - Print masternode status\n"
                "  genkey       - Generate new masternodeprivkey\n"
                "  enforce      - Enforce masternode payments\n"
                "  outputs      - Print masternode compatible outputs\n"
                "  start        - Start masternode configured in darkcoin.conf\n"
                "  start-alias  - Start single masternode by assigned alias configured in masternode.conf\n"
                "  start-many   - Start all masternodes configured in masternode.conf\n"
                "  stop         - Stop masternode configured in darkcoin.conf\n"
                "  stop-alias   - Stop single masternode by assigned alias configured in masternode.conf\n"
                "  stop-many    - Stop all masternodes configured in masternode.conf\n"
                "  list         - Print list of all known masternodes (see masternodelist for more info)\n"
                "  list-conf    - Print masternode.conf in JSON format\n"
                "  winners      - Print list of masternode winners\n"
                );

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

        std::string errorMessage;
        if(!activeMasternode.StopMasterNode(errorMessage)) {
        	return "stop failed: " + errorMessage;
        }
        pwalletMain->Lock();

        if(activeMasternode.status == MASTERNODE_STOPPED) return "successfully stopped masternode";
        if(activeMasternode.status == MASTERNODE_NOT_CAPABLE) return "not capable masternode";

        return "unknown";
    }

    if (strCommand == "stop-alias")
    {
	    if (params.size() < 2){
			throw runtime_error(
			"command needs at least 2 parameters\n");
	    }

	    std::string alias = params[1].get_str().c_str();

    	if(pwalletMain->IsLocked()) {
    		SecureString strWalletPass;
    	    strWalletPass.reserve(100);

			if (params.size() == 3){
				strWalletPass = params[2].get_str().c_str();
			} else {
				throw runtime_error(
				"Your wallet is locked, passphrase is required\n");
			}

			if(!pwalletMain->Unlock(strWalletPass)){
				return "incorrect passphrase";
			}
        }

    	bool found = false;

		Object statusObj;
		statusObj.push_back(Pair("alias", alias));

    	BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
    		if(mne.getAlias() == alias) {
    			found = true;
    			std::string errorMessage;
    			bool result = activeMasternode.StopMasterNode(mne.getIp(), mne.getPrivKey(), errorMessage);

				statusObj.push_back(Pair("result", result ? "successful" : "failed"));
    			if(!result) {
   					statusObj.push_back(Pair("errorMessage", errorMessage));
   				}
    			break;
    		}
    	}

    	if(!found) {
    		statusObj.push_back(Pair("result", "failed"));
    		statusObj.push_back(Pair("errorMessage", "could not find alias in config. Verify with list-conf."));
    	}

    	pwalletMain->Lock();
    	return statusObj;
    }

    if (strCommand == "stop-many")
    {
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

		int total = 0;
		int successful = 0;
		int fail = 0;


		Object resultsObj;

		BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
			total++;

			std::string errorMessage;
			bool result = activeMasternode.StopMasterNode(mne.getIp(), mne.getPrivKey(), errorMessage);

			Object statusObj;
			statusObj.push_back(Pair("alias", mne.getAlias()));
			statusObj.push_back(Pair("result", result ? "successful" : "failed"));

			if(result) {
				successful++;
			} else {
				fail++;
				statusObj.push_back(Pair("errorMessage", errorMessage));
			}

			resultsObj.push_back(Pair("status", statusObj));
		}
		pwalletMain->Lock();

		Object returnObj;
		returnObj.push_back(Pair("overall", "Successfully stopped " + boost::lexical_cast<std::string>(successful) + " masternodes, failed to stop " +
				boost::lexical_cast<std::string>(fail) + ", total " + boost::lexical_cast<std::string>(total)));
		returnObj.push_back(Pair("detail", resultsObj));

		return returnObj;

    }

    if (strCommand == "list")
    {
        Array newParams(params.size() - 1);
        std::copy(params.begin() + 1, params.end(), newParams.begin());
        return masternodelist(newParams, fHelp);
    }

    if (strCommand == "count")
    {
        if (params.size() > 2){
            throw runtime_error(
            "too many parameters\n");
        }
        if (params.size() == 2)
        {
            if(params[1] == "enabled") return mnodeman.CountEnabled();
            if(params[1] == "both") return boost::lexical_cast<std::string>(mnodeman.CountEnabled()) + " / " + boost::lexical_cast<std::string>(mnodeman.size());
        }
        return mnodeman.size();
    }

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

        if(activeMasternode.status != MASTERNODE_REMOTELY_ENABLED && activeMasternode.status != MASTERNODE_IS_CAPABLE){
            activeMasternode.status = MASTERNODE_NOT_PROCESSED; // TODO: consider better way
            std::string errorMessage;
            activeMasternode.ManageStatus();
            pwalletMain->Lock();
        }

        if(activeMasternode.status == MASTERNODE_REMOTELY_ENABLED) return "masternode started remotely";
        if(activeMasternode.status == MASTERNODE_INPUT_TOO_NEW) return "masternode input must have at least 15 confirmations";
        if(activeMasternode.status == MASTERNODE_STOPPED) return "masternode is stopped";
        if(activeMasternode.status == MASTERNODE_IS_CAPABLE) return "successfully started masternode";
        if(activeMasternode.status == MASTERNODE_NOT_CAPABLE) return "not capable masternode: " + activeMasternode.notCapableReason;
        if(activeMasternode.status == MASTERNODE_SYNC_IN_PROCESS) return "sync in process. Must wait until client is synced to start.";

        return "unknown";
    }

    if (strCommand == "start-alias")
    {
	    if (params.size() < 2){
			throw runtime_error(
			"command needs at least 2 parameters\n");
	    }

	    std::string alias = params[1].get_str().c_str();

    	if(pwalletMain->IsLocked()) {
    		SecureString strWalletPass;
    	    strWalletPass.reserve(100);

			if (params.size() == 3){
				strWalletPass = params[2].get_str().c_str();
			} else {
				throw runtime_error(
				"Your wallet is locked, passphrase is required\n");
			}

			if(!pwalletMain->Unlock(strWalletPass)){
				return "incorrect passphrase";
			}
        }

    	bool found = false;

		Object statusObj;
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
    		statusObj.push_back(Pair("errorMessage", "could not find alias in config. Verify with list-conf."));
    	}

    	pwalletMain->Lock();
    	return statusObj;

    }

    if (strCommand == "start-many")
    {
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

		std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
		mnEntries = masternodeConfig.getEntries();

		int total = 0;
		int successful = 0;
		int fail = 0;

		Object resultsObj;

		BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
			total++;

			std::string errorMessage;
			bool result = activeMasternode.Register(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), errorMessage);

			Object statusObj;
			statusObj.push_back(Pair("alias", mne.getAlias()));
            statusObj.push_back(Pair("result", result ? "successful" : "failed"));

			if(result) {
				successful++;
			} else {
				fail++;
				statusObj.push_back(Pair("errorMessage", errorMessage));
			}

			resultsObj.push_back(Pair("status", statusObj));
		}
		pwalletMain->Lock();

		Object returnObj;
		returnObj.push_back(Pair("overall", "Successfully started " + boost::lexical_cast<std::string>(successful) + " masternodes, failed to start " +
				boost::lexical_cast<std::string>(fail) + ", total " + boost::lexical_cast<std::string>(total)));
		returnObj.push_back(Pair("detail", resultsObj));

		return returnObj;
    }

    if (strCommand == "debug")
    {
        if(activeMasternode.status == MASTERNODE_REMOTELY_ENABLED) return "masternode started remotely";
        if(activeMasternode.status == MASTERNODE_INPUT_TOO_NEW) return "masternode input must have at least 15 confirmations";
        if(activeMasternode.status == MASTERNODE_IS_CAPABLE) return "successfully started masternode";
        if(activeMasternode.status == MASTERNODE_STOPPED) return "masternode is stopped";
        if(activeMasternode.status == MASTERNODE_NOT_CAPABLE) return "not capable masternode: " + activeMasternode.notCapableReason;
        if(activeMasternode.status == MASTERNODE_SYNC_IN_PROCESS) return "sync in process. Must wait until client is synced to start.";

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
        CMasternode* winner = mnodeman.GetCurrentMasterNode(1);
        if(winner) {
            Object obj;
            CScript pubkey;
            pubkey.SetDestination(winner->pubkey.GetID());
            CTxDestination address1;
            ExtractDestination(pubkey, address1);
            CBitcoinAddress address2(address1);

            obj.push_back(Pair("IP:port",       winner->addr.ToString().c_str()));
            obj.push_back(Pair("protocol",      (int64_t)winner->protocolVersion));
            obj.push_back(Pair("vin",           winner->vin.prevout.hash.ToString().c_str()));
            obj.push_back(Pair("pubkey",        address2.ToString().c_str()));
            obj.push_back(Pair("lastseen",      (int64_t)winner->lastTimeSeen));
            obj.push_back(Pair("activeseconds", (int64_t)(winner->lastTimeSeen - winner->sigTime)));
            return obj;
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

    if(strCommand == "list-conf")
    {
    	std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
    	mnEntries = masternodeConfig.getEntries();

        Object resultObj;

        BOOST_FOREACH(CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
    		Object mnObj;
    		mnObj.push_back(Pair("alias", mne.getAlias()));
    		mnObj.push_back(Pair("address", mne.getIp()));
    		mnObj.push_back(Pair("privateKey", mne.getPrivKey()));
    		mnObj.push_back(Pair("txHash", mne.getTxHash()));
    		mnObj.push_back(Pair("outputIndex", mne.getOutputIndex()));
    		resultObj.push_back(Pair("masternode", mnObj));
    	}

    	return resultObj;
    }

    if (strCommand == "outputs"){
        // Find possible candidates
        vector<COutput> possibleCoins = activeMasternode.SelectCoinsMasternode();

        Object obj;
        BOOST_FOREACH(COutput& out, possibleCoins) {
            obj.push_back(Pair(out.tx->GetHash().ToString().c_str(), boost::lexical_cast<std::string>(out.i)));
        }

        return obj;

    }

    return Value::null;
}

Value masternodelist(const Array& params, bool fHelp)
{
    std::string strMode = "active";
    std::string strFilter = "";

    if (params.size() >= 1) strMode = params[0].get_str();
    if (params.size() == 2) strFilter = params[1].get_str();

    if (fHelp ||
            (strMode != "active" && strMode != "vin" && strMode != "pubkey" && strMode != "lastseen"
             && strMode != "activeseconds" && strMode != "rank" && strMode != "protocol" && strMode != "full"))
    {
        throw runtime_error(
                "masternodelist ( \"mode\" \"filter\" )\n"
                "Get a list of masternodes in different modes\n"
                "\nArguments:\n"
                "1. \"mode\"      (string, optional, defauls = active) The mode to run list in\n"
                "2. \"filter\"    (string, optional) Filter results, can be applied in few modes only\n"
                "\nAvailable modes:\n"
                "  active         - Print '1' if active and '0' otherwise (can be filtered, exact match)\n"
                "  activeseconds  - Print number of seconds masternode recognized by the network as enabled\n"
                "  full           - Print info in format 'active | protocol | pubkey | vin | lastseen | activeseconds' (can be filtered, partial match)\n"
                "  lastseen       - Print timestamp of when a masternode was last seen on the network\n"
                "  protocol       - Print protocol of a masternode (can be filtered, exact match)\n"
                "  pubkey         - Print public key associated with a masternode (can be filtered, partial match)\n"
                "  rank           - Print rank of a masternode based on current block\n"
                "  vin            - Print vin associated with a masternode (can be filtered, partial match)\n"
                );
    }

    Object obj;
    std::vector<CMasternode> vMasternodes = mnodeman.GetFullMasternodeVector();
    BOOST_FOREACH(CMasternode& mn, vMasternodes) {

        std::string strAddr = mn.addr.ToString().c_str();
        if(strMode == "active"){
            if(strFilter !="" && strFilter != boost::lexical_cast<std::string>(mn.IsEnabled()) &&
                mn.addr.ToString().find(strFilter) == string::npos) continue;
            obj.push_back(Pair(strAddr,       (int)mn.IsEnabled()));
        } else if (strMode == "vin") {
            if(strFilter !="" && mn.vin.prevout.hash.ToString().find(strFilter) == string::npos &&
                mn.addr.ToString().find(strFilter) == string::npos) continue;
            obj.push_back(Pair(strAddr,       mn.vin.prevout.hash.ToString().c_str()));
        } else if (strMode == "pubkey") {
            CScript pubkey;
            pubkey.SetDestination(mn.pubkey.GetID());
            CTxDestination address1;
            ExtractDestination(pubkey, address1);
            CBitcoinAddress address2(address1);

            if(strFilter !="" && address2.ToString().find(strFilter) == string::npos &&
                mn.addr.ToString().find(strFilter) == string::npos) continue;
            obj.push_back(Pair(strAddr,       address2.ToString().c_str()));
        } else if (strMode == "protocol") {
            if(strFilter !="" && strFilter != boost::lexical_cast<std::string>(mn.protocolVersion) &&
                mn.addr.ToString().find(strFilter) == string::npos) continue;
            obj.push_back(Pair(strAddr,       (int64_t)mn.protocolVersion));
        } else if (strMode == "lastseen") {
            if(strFilter !="" && mn.addr.ToString().find(strFilter) == string::npos) continue;

            obj.push_back(Pair(strAddr,       (int64_t)mn.lastTimeSeen));
        } else if (strMode == "activeseconds") {
            if(strFilter !="" && mn.addr.ToString().find(strFilter) == string::npos) continue;

            obj.push_back(Pair(strAddr,       (int64_t)(mn.lastTimeSeen - mn.sigTime)));
        } else if (strMode == "rank") {
            if(strFilter !="" && mn.addr.ToString().find(strFilter) == string::npos) continue;
            obj.push_back(Pair(strAddr,       (int)(mnodeman.GetMasternodeRank(mn.vin, chainActive.Tip()->nHeight))));
        } else if (strMode == "full") {
            CScript pubkey;
            pubkey.SetDestination(mn.pubkey.GetID());
            CTxDestination address1;
            ExtractDestination(pubkey, address1);
            CBitcoinAddress address2(address1);

            std::ostringstream stringStream;
            stringStream << (mn.IsEnabled() ? "1" : "0") << " | " <<
                           mn.protocolVersion << " | " <<
                           address2.ToString() << " | " <<
                           mn.vin.prevout.hash.ToString() << " | " <<
                           mn.lastTimeSeen << " | " <<
                           (mn.lastTimeSeen - mn.sigTime);
            std::string output = stringStream.str();
            stringStream << " " << strAddr;
            if(strFilter !="" && stringStream.str().find(strFilter) == string::npos) continue;
            obj.push_back(Pair(strAddr, output));
        }
    }
    return obj;

}
