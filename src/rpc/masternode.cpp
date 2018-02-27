// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2015-2017 The Liberta developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activemasternode.h"
#include "db.h"
#include "init.h"
#include "main.h"
#include "masternode-budget.h"
#include "masternode-payments.h"
#include "masternodeconfig.h"
#include "masternodeman.h"
#include "rpc/server.h"
#include "utilmoneystr.h"

#include <fstream>
#include <iomanip>

void SendMoney(const CTxDestination& address, CAmount nValue, CWalletTx& wtxNew, AvailableCoinsType coin_type = ALL_COINS)
{
    // Check amount
    if (nValue <= 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid amount");

    if (nValue > pwalletMain->GetBalance())
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");

    std::string strError;
    if (pwalletMain->IsLocked()) {
        strError = "Error: Wallet locked, unable to create transaction!";
        LogPrintf("SendMoney() : %s", strError);
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }

    // Parse Liberta address
    CScript scriptPubKey = GetScriptForDestination(address);

    // Create and send the transaction
    CReserveKey reservekey(pwalletMain);
    CAmount nFeeRequired;
    std::vector<CRecipient> vecSend;
    int nChangePosRet = -1;
    CRecipient recipient = {scriptPubKey, nValue, false};
    vecSend.push_back(recipient);
    if (!pwalletMain->CreateTransaction(vecSend, wtxNew, reservekey, nFeeRequired, nChangePosRet, strError, NULL, coin_type)) {
        if (nValue + nFeeRequired > pwalletMain->GetBalance())
            strError = strprintf("Error: This transaction requires a transaction fee of at least %s because of its amount, complexity, or use of recently received funds!", FormatMoney(nFeeRequired));
        LogPrintf("SendMoney() : %s\n", strError);
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
    }
    if (!pwalletMain->CommitTransaction(wtxNew, reservekey))
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");
}

UniValue obfuscation(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() == 0)
        throw std::runtime_error(
            "obfuscation <libertaaddress> <amount>\n"
            "libertaaddress, reset, or auto (AutoDenominate)"
            "<amount> is a real and will be rounded to the next 0.1" +
            HelpRequiringPassphrase());

    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");

    if (params[0].get_str() == "auto") {
        if (fMasterNode)
            return "ObfuScation is not supported from masternodes";

        return "DoAutomaticDenominating " + (obfuScationPool.DoAutomaticDenominating() ? "successful" : ("failed: " + obfuScationPool.GetStatus()));
    }

    if (params[0].get_str() == "reset") {
        obfuScationPool.Reset();
        return "successfully reset obfuscation";
    }

    if (params.size() != 2)
        throw std::runtime_error(
            "obfuscation <libertaaddress> <amount>\n"
            "libertaaddress, denominate, or auto (AutoDenominate)"
            "<amount> is a real and will be rounded to the next 0.1" +
            HelpRequiringPassphrase());

    CLibertaAddress address(params[0].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Liberta address");

    // Amount
    CAmount nAmount = AmountFromValue(params[1]);

    // Wallet comments
    CWalletTx wtx;
    //    string strError = pwalletMain->SendMoneyToDestination(address.Get(), nAmount, wtx, ONLY_DENOMINATED);
    SendMoney(address.Get(), nAmount, wtx, ONLY_DENOMINATED);
    //    if (strError != "")
    //        throw JSONRPCError(RPC_WALLET_ERROR, strError);

    return wtx.GetHash().GetHex();
}


UniValue getpoolinfo(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw std::runtime_error(
            "getpoolinfo\n"
            "Returns an object containing anonymous pool-related information.");

    UniValue obj(UniValue::VOBJ);
    obj.push_back(Pair("current_masternode", mnodeman.GetCurrentMasterNode()->addr.ToString()));
    obj.push_back(Pair("state", obfuScationPool.GetState()));
    obj.push_back(Pair("entries", obfuScationPool.GetEntriesCount()));
    obj.push_back(Pair("entries_accepted", obfuScationPool.GetCountEntriesAccepted()));
    return obj;
}


UniValue masternode(const UniValue& params, bool fHelp)
{
    std::string strCommand;
    if (params.size() >= 1)
        strCommand = params[0].get_str();

    if (fHelp ||
        (strCommand != "start" && strCommand != "start-alias" && strCommand != "start-many" && strCommand != "start-all" && strCommand != "start-missing" &&
            strCommand != "start-disabled" && strCommand != "list" && strCommand != "list-conf" && strCommand != "count" && strCommand != "enforce" &&
            strCommand != "debug" && strCommand != "current" && strCommand != "winners" && strCommand != "genkey" && strCommand != "connect" &&
            strCommand != "outputs" && strCommand != "status" && strCommand != "calcscore"))
        throw std::runtime_error(
            "masternode \"command\"... ( \"passphrase\" )\n"
            "Set of commands to execute masternode related actions\n"
            "\nArguments:\n"
            "1. \"command\"        (string or set of strings, required) The command to execute\n"
            "2. \"passphrase\"     (string, optional) The wallet passphrase\n"
            "\nAvailable commands:\n"
            "  count        - Print number of all known masternodes (optional: 'obf', 'enabled', 'all', 'qualify')\n"
            "  current      - Print info on current masternode winner\n"
            "  debug        - Print masternode status\n"
            "  genkey       - Generate new masternodeprivkey\n"
            "  enforce      - Enforce masternode payments\n"
            "  outputs      - Print masternode compatible outputs\n"
            "  start        - Start masternode configured in liberta.conf\n"
            "  start-alias  - Start single masternode by assigned alias configured in masternode.conf\n"
            "  start-<mode> - Start masternodes configured in masternode.conf (<mode>: 'all', 'missing', 'disabled')\n"
            "  status       - Print masternode status information\n"
            "  list         - Print list of all known masternodes (see masternodelist for more info)\n"
            "  list-conf    - Print masternode.conf in JSON format\n"
            "  winners      - Print list of masternode winners\n");

   /* if (strCommand == "list") {
        Array newParams(params.size() - 1);
        std::copy(params.begin() + 1, params.end(), newParams.begin());
        return masternodelist(newParams, fHelp);
    }*/

    if (strCommand == "budget") {
        return "Show budgets";
    }

    if (strCommand == "connect") {
        std::string strAddress = "";
        if (params.size() == 2) {
            strAddress = params[1].get_str();
        } else {
            throw std::runtime_error("Masternode address required\n");
        }

        CService addr = CService(strAddress);

        CNode* pnode = ConnectNode((CAddress)addr, NULL);
        if (pnode) {
            pnode->Release();
            return "successfully connected";
        } else {
            throw std::runtime_error("error connecting\n");
        }
    }

    if (strCommand == "count") {
        if (params.size() > 2) {
            throw std::runtime_error("too many parameters\n");
        }
        if (params.size() == 2) {
            int nCount = 0;

            if (chainActive.Tip())
                mnodeman.GetNextMasternodeInQueueForPayment(chainActive.Tip()->nHeight, true, nCount);

            if (params[1].get_str() == "obf") return mnodeman.CountEnabled(MIN_POOL_PEER_PROTO_VERSION);
            if (params[1].get_str() == "enabled") return mnodeman.CountEnabled();
            if (params[1].get_str() == "qualify") return nCount;
            if (params[1].get_str() == "all") return strprintf("Total: %d (OBF Compatible: %d / Enabled: %d / Qualify: %d)",
                mnodeman.size(),
                mnodeman.CountEnabled(MIN_POOL_PEER_PROTO_VERSION),
                mnodeman.CountEnabled(),
                nCount);
        }
        return mnodeman.size();
    }

    if (strCommand == "current") {
        CMasternode* winner = mnodeman.GetCurrentMasterNode(1);
        if (winner) {
            UniValue obj(UniValue::VOBJ);

            obj.push_back(Pair("IP:port", winner->addr.ToString()));
            obj.push_back(Pair("protocol", (int64_t)winner->protocolVersion));
            obj.push_back(Pair("vin", winner->vin.prevout.hash.ToString()));
            obj.push_back(Pair("pubkey", CLibertaAddress(winner->pubKeyCollateralAddress.GetID()).ToString()));
            obj.push_back(Pair("lastseen", (winner->lastPing == CMasternodePing()) ? winner->sigTime : (int64_t)winner->lastPing.sigTime));
            obj.push_back(Pair("activeseconds", (winner->lastPing == CMasternodePing()) ? 0 : (int64_t)(winner->lastPing.sigTime - winner->sigTime)));
            return obj;
        }

        return "unknown";
    }

    if (strCommand == "debug") {
        if (activeMasternode.status != ACTIVE_MASTERNODE_INITIAL || !masternodeSync.IsSynced())
            return activeMasternode.GetStatus();

        CTxIn vin = CTxIn();
        CPubKey pubkey;
        CKey key;
        bool found = activeMasternode.GetMasterNodeVin(vin, pubkey, key);
        if (!found) {
            throw std::runtime_error("Missing masternode input, please look at the documentation for instructions on masternode creation\n");
        } else {
            return activeMasternode.GetStatus();
        }
    }

    if (strCommand == "enforce") {
        return (uint64_t)enforceMasternodePaymentsTime;
    }

    if (strCommand == "start") {
        if (!fMasterNode) throw std::runtime_error("you must set masternode=1 in the configuration\n");

        if (pwalletMain->IsLocked()) {
            SecureString strWalletPass;
            strWalletPass.reserve(100);

            if (params.size() == 2) {
                strWalletPass = params[1].get_str().c_str();
            } else {
                throw std::runtime_error("Your wallet is locked, passphrase is required\n");
            }

            if (!pwalletMain->Unlock(strWalletPass)) {
                throw std::runtime_error("incorrect passphrase\n");
            }
        }

        if (activeMasternode.status != ACTIVE_MASTERNODE_STARTED) {
            activeMasternode.status = ACTIVE_MASTERNODE_INITIAL; // TODO: consider better way
            activeMasternode.ManageStatus();
            pwalletMain->Lock();
        }

        return activeMasternode.GetStatus();
    }

    if (strCommand == "start-alias") {
        if (params.size() < 2) {
            throw std::runtime_error("command needs at least 2 parameters\n");
        }

        std::string alias = params[1].get_str();

        if (pwalletMain->IsLocked()) {
            SecureString strWalletPass;
            strWalletPass.reserve(100);

            if (params.size() == 3) {
                strWalletPass = params[2].get_str().c_str();
            } else {
                throw std::runtime_error("Your wallet is locked, passphrase is required\n");
            }

            if (!pwalletMain->Unlock(strWalletPass)) {
                throw std::runtime_error("incorrect passphrase\n");
            }
        }

        bool found = false;

        UniValue statusObj(UniValue::VOBJ);
        statusObj.push_back(Pair("alias", alias));

        BOOST_FOREACH (CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
            if (mne.getAlias() == alias) {
                found = true;
                std::string errorMessage;

                bool result = activeMasternode.Register(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), errorMessage);

                statusObj.push_back(Pair("result", result ? "successful" : "failed"));
                if (!result) {
                    statusObj.push_back(Pair("errorMessage", errorMessage));
                }
                break;
            }
        }

        if (!found) {
            statusObj.push_back(Pair("result", "failed"));
            statusObj.push_back(Pair("errorMessage", "could not find alias in config. Verify with list-conf."));
        }

        pwalletMain->Lock();
        return statusObj;
    }

    if (strCommand == "start-many" || strCommand == "start-all" || strCommand == "start-missing" || strCommand == "start-disabled") {
        if (pwalletMain->IsLocked()) {
            SecureString strWalletPass;
            strWalletPass.reserve(100);

            if (params.size() == 2) {
                strWalletPass = params[1].get_str().c_str();
            } else {
                throw std::runtime_error("Your wallet is locked, passphrase is required\n");
            }

            if (!pwalletMain->Unlock(strWalletPass)) {
                throw std::runtime_error("incorrect passphrase\n");
            }
        }

        if ((strCommand == "start-missing" || strCommand == "start-disabled") &&
            (masternodeSync.RequestedMasternodeAssets <= MASTERNODE_SYNC_LIST ||
                masternodeSync.RequestedMasternodeAssets == MASTERNODE_SYNC_FAILED)) {
            throw std::runtime_error("You can't use this command until masternode list is synced\n");
        }

        std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        int successful = 0;
        int failed = 0;

        UniValue resultsObj(UniValue::VOBJ);

        BOOST_FOREACH (CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
            std::string errorMessage;

            CTxIn vin = CTxIn(uint256S(mne.getTxHash()), uint32_t(atoi(mne.getOutputIndex().c_str())));
            CMasternode* pmn = mnodeman.Find(vin);

            if (strCommand == "start-missing" && pmn) continue;
            if (strCommand == "start-disabled" && pmn && pmn->IsEnabled()) continue;

            bool result = activeMasternode.Register(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), errorMessage);

            UniValue statusObj(UniValue::VOBJ);
            statusObj.push_back(Pair("alias", mne.getAlias()));
            statusObj.push_back(Pair("result", result ? "successful" : "failed"));

            if (result) {
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

    if (strCommand == "create") {
        throw std::runtime_error("Not implemented yet, please look at the documentation for instructions on masternode creation\n");
    }

    if (strCommand == "genkey") {
        CKey secret;
        secret.MakeNewKey(false);

        return CLibertaSecret(secret).ToString();
    }

    if (strCommand == "list-conf") {
        std::vector<CMasternodeConfig::CMasternodeEntry> mnEntries;
        mnEntries = masternodeConfig.getEntries();

        UniValue resultObj(UniValue::VOBJ);

        BOOST_FOREACH (CMasternodeConfig::CMasternodeEntry mne, masternodeConfig.getEntries()) {
            CTxIn vin = CTxIn(uint256S(mne.getTxHash()), uint32_t(atoi(mne.getOutputIndex().c_str())));
            CMasternode* pmn = mnodeman.Find(vin);

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

    if (strCommand == "outputs") {
        // Find possible candidates
        std::vector<COutput> possibleCoins = activeMasternode.SelectCoinsMasternode();

        UniValue obj(UniValue::VOBJ);
        BOOST_FOREACH (COutput& out, possibleCoins) {
            obj.push_back(Pair(out.tx->GetHash().ToString(), strprintf("%d", out.i)));
        }

        return obj;
    }

    if (strCommand == "status") {
        if (!fMasterNode) throw std::runtime_error("This is not a masternode\n");

        UniValue mnObj(UniValue::VOBJ);
        CMasternode* pmn = mnodeman.Find(activeMasternode.vin);

        mnObj.push_back(Pair("vin", activeMasternode.vin.ToString()));
        mnObj.push_back(Pair("service", activeMasternode.service.ToString()));
        if (pmn) mnObj.push_back(Pair("pubkey", CLibertaAddress(pmn->pubKeyCollateralAddress.GetID()).ToString()));
        mnObj.push_back(Pair("status", activeMasternode.GetStatus()));
        return mnObj;
    }

    if (strCommand == "winners") {
        int nLast = 10;

        if (params.size() >= 2) {
            nLast = atoi(params[1].get_str());
        }

        UniValue obj(UniValue::VOBJ);

        for (int nHeight = chainActive.Tip()->nHeight - nLast; nHeight < chainActive.Tip()->nHeight + 20; nHeight++) {
            obj.push_back(Pair(strprintf("%d", nHeight), GetRequiredPaymentsString(nHeight)));
        }

        return obj;
    }

    /*
        Shows which masternode wins by score each block
    */
    if (strCommand == "calcscore") {
        int nLast = 10;

        if (params.size() >= 2) {
            nLast = atoi(params[1].get_str());
        }
        UniValue obj(UniValue::VOBJ);

        std::vector<CMasternode> vMasternodes = mnodeman.GetFullMasternodeVector();
        for (int nHeight = chainActive.Tip()->nHeight - nLast; nHeight < chainActive.Tip()->nHeight + 20; nHeight++) {
            uint256 nHigh;
            CMasternode* pBestMasternode = NULL;
            BOOST_FOREACH (CMasternode& mn, vMasternodes) {
                uint256 n = mn.CalculateScore(1, nHeight - 100);
                if (nHigh < n) {
                    nHigh = n;
                    pBestMasternode = &mn;
                }
            }
            if (pBestMasternode)
                obj.push_back(Pair(strprintf("%d", nHeight), pBestMasternode->vin.prevout.ToStringShort().c_str()));
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
        (strMode != "status" && strMode != "vin" && strMode != "pubkey" && strMode != "lastseen" && strMode != "activeseconds" && strMode != "rank" && strMode != "addr" && strMode != "protocol" && strMode != "full" && strMode != "lastpaid")) {
        throw std::runtime_error(
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
            "                   (can be additionally filtered, partial match)\n");
    }

    UniValue obj(UniValue::VOBJ);
    if (strMode == "rank") {
        std::vector<std::pair<int, CMasternode> > vMasternodeRanks = mnodeman.GetMasternodeRanks(chainActive.Tip()->nHeight);
        BOOST_FOREACH (PAIRTYPE(int, CMasternode) & s, vMasternodeRanks) {
            std::string strVin = s.second.vin.prevout.ToStringShort();
            if (strFilter != "" && strVin.find(strFilter) == std::string::npos) continue;
            obj.push_back(Pair(strVin, s.first));
        }
    } else {
        std::vector<CMasternode> vMasternodes = mnodeman.GetFullMasternodeVector();
        BOOST_FOREACH (CMasternode& mn, vMasternodes) {
            std::string strVin = mn.vin.prevout.ToStringShort();
            if (strMode == "activeseconds") {
                if (strFilter != "" && strVin.find(strFilter) == std::string::npos) continue;
                obj.push_back(Pair(strVin, (int64_t)(mn.lastPing.sigTime - mn.sigTime)));
            } else if (strMode == "addr") {
                if (strFilter != "" && mn.vin.prevout.hash.ToString().find(strFilter) == std::string::npos &&
                    strVin.find(strFilter) == std::string::npos) continue;
                obj.push_back(Pair(strVin, mn.addr.ToString()));
            } else if (strMode == "full") {
                std::ostringstream addrStream;
                addrStream << std::setw(21) << strVin;

                std::ostringstream stringStream;
                stringStream << std::setw(9) << mn.Status() << " " << mn.protocolVersion << " " << CLibertaAddress(mn.pubKeyCollateralAddress.GetID()).ToString() << " " << std::setw(21) << mn.addr.ToString() << " " << (int64_t)mn.lastPing.sigTime << " " << std::setw(8) << (int64_t)(mn.lastPing.sigTime - mn.sigTime) << " " << (int64_t)mn.GetLastPaid();
                std::string output = stringStream.str();
                stringStream << " " << strVin;
                if (strFilter != "" && stringStream.str().find(strFilter) == std::string::npos &&
                    strVin.find(strFilter) == std::string::npos) continue;
                obj.push_back(Pair(addrStream.str(), output));
            } else if (strMode == "lastseen") {
                if (strFilter != "" && strVin.find(strFilter) == std::string::npos) continue;
                obj.push_back(Pair(strVin, (int64_t)mn.lastPing.sigTime));
            } else if (strMode == "lastpaid") {
                if (strFilter != "" && mn.vin.prevout.hash.ToString().find(strFilter) == std::string::npos &&
                    strVin.find(strFilter) == std::string::npos) continue;
                obj.push_back(Pair(strVin, (int64_t)mn.GetLastPaid()));
            } else if (strMode == "protocol") {
                if (strFilter != "" && strFilter != strprintf("%d", mn.protocolVersion) &&
                    strVin.find(strFilter) == std::string::npos) continue;
                obj.push_back(Pair(strVin, (int64_t)mn.protocolVersion));
            } else if (strMode == "pubkey") {
                CLibertaAddress address(mn.pubKeyCollateralAddress.GetID());

                if (strFilter != "" && address.ToString().find(strFilter) == std::string::npos &&
                    strVin.find(strFilter) == std::string::npos) continue;
                obj.push_back(Pair(strVin, address.ToString()));
            } else if (strMode == "status") {
                std::string strStatus = mn.Status();
                if (strFilter != "" && strVin.find(strFilter) == std::string::npos && strStatus.find(strFilter) == std::string::npos) continue;
                obj.push_back(Pair(strVin, strStatus));
            }
        }
    }
    return obj;
}
