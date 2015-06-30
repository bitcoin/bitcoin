// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "corewallet/corewallet.h"
#include "corewallet/corewallet_db.h"
#include "corewallet/corewallet_wallet.h"
#include "rpcserver.h"
#include "script/script.h"
#include "ui_interface.h"
#include "univalue/univalue.h"
#include "util.h"
#include "validationinterface.h"

#include <string>

#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>

namespace CoreWallet {

const static std::string DEFAULT_WALLETS_METADATA_FILE = "multiwallet.dat";
static Manager *managerSharedInstance;

//implemented in corewallet_rpc.cpp
extern void ExecuteRPC(const std::string& strMethod, const UniValue& params, UniValue& result, bool& accept);


bool CheckFilenameString(const std::string& str)
{
    static std::string safeChars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890._-");
    std::string strResult;
    for (std::string::size_type i = 0; i < str.size(); i++)
    {
        if (safeChars.find(str[i]) == std::string::npos)
            return false;
    }
    return true;
}
    
void AppendHelpMessageString(std::string& strUsage, bool debugHelp)
{
    if (debugHelp)
        return;
    
    strUsage += HelpMessageGroup(_("CoreWallet options:"));
    strUsage += HelpMessageOpt("-disablecorewallet", _("Do not load the wallet and disable wallet RPC calls"));
}

Manager::Manager()
{
    ReadWalletLists();
}

void Manager::ReadWalletLists()
{
    CAutoFile multiwalletFile(fopen((GetDataDir() / DEFAULT_WALLETS_METADATA_FILE).string().c_str(), "rb"), SER_DISK, CLIENT_VERSION);
    if (!multiwalletFile.IsNull())
    {
        try {
            LOCK(cs_mapWallets);
            multiwalletFile >> mapWallets;
        } catch (const std::exception&) {
            LogPrintf("CoreWallet: could not read multiwallet metadata file (non-fatal)");
        }
    }
}

void Manager::WriteWalletList()
{
    CAutoFile multiwalletFile(fopen((GetDataDir() / DEFAULT_WALLETS_METADATA_FILE).string().c_str(), "wb"), SER_DISK, CLIENT_VERSION);
    if (!multiwalletFile.IsNull())
    {
        LOCK(cs_mapWallets);
        multiwalletFile << mapWallets;
    }
}

void LoadAsModule(std::string& warningString, std::string& errorString, bool& stopInit)
{
    GetManager();
}

Wallet* Manager::AddNewWallet(const std::string& walletID)
{
    Wallet *newWallet = NULL;
    LOCK(cs_mapWallets);
    {
        if (mapWallets.find(walletID) != mapWallets.end())
            throw std::runtime_error(_("walletid already exists"));
        
        if (!CheckFilenameString(walletID))
            throw std::runtime_error(_("wallet ids can only contain A-Za-z0-9._- chars"));
        
        newWallet = new Wallet(walletID);
        mapWallets[walletID] = WalletModel(walletID, newWallet);
    }

    WriteWalletList();
    return newWallet;
}

Wallet* Manager::GetWalletWithID(const std::string& walletIDIn)
{
    std::string walletID = walletIDIn;

    LOCK(cs_mapWallets);
    {
        if (walletID == "" && mapWallets.size() == 1)
            walletID = mapWallets.begin()->first;

        if (mapWallets.find(walletID) != mapWallets.end())
        {
            if (!mapWallets[walletID].pWallet) //is it closed?
                mapWallets[walletID].pWallet = new Wallet(walletID);

            return mapWallets[walletID].pWallet;
        }
    }
    
    return NULL;
}

std::vector<std::string> Manager::GetWalletIDs()
{
    std::vector<std::string> vIDs;
    std::pair<std::string, WalletModel> walletAndMetadata; // what a map<int, int> is made of

    LOCK(cs_mapWallets);
    {
        BOOST_FOREACH(walletAndMetadata, mapWallets) {
            vIDs.push_back(walletAndMetadata.first);
        }
    }
    return vIDs;
}

void Dealloc()
{
    if (managerSharedInstance)
    {
        UnregisterValidationInterface(managerSharedInstance);
        delete managerSharedInstance;
        managerSharedInstance = NULL;
    }
}

Manager* GetManager()
{
    if (!managerSharedInstance)
    {
        managerSharedInstance = new Manager();
        RegisterValidationInterface(managerSharedInstance);
    }
    return managerSharedInstance;
}

void Manager::SyncTransaction(const CTransaction& tx, const CBlock* pblock)
{
    LOCK(cs_mapWallets);
    {
        std::pair<std::string, WalletModel> walletAndMetadata;
        BOOST_FOREACH(walletAndMetadata, mapWallets) {
            //TODO: looks ugly
            //Open the wallet within the SyncTransaction call is probably a bad idea, need to be changed
            if (!mapWallets[walletAndMetadata.first].pWallet) //is it closed?
                mapWallets[walletAndMetadata.first].pWallet = new Wallet(walletAndMetadata.first);


            mapWallets[walletAndMetadata.first].pWallet->SyncTransaction(tx, pblock);
        }
    }
}

void GetScriptForMining(boost::shared_ptr<CReserveScript> &script)
{
    Wallet *wallet = CoreWallet::GetManager()->GetWalletWithID("");
    if (wallet)
        wallet->GetScriptForMining(script);
}

void RegisterSignals()
{
    AddJSONRPCURISchema("/corewallet");
    RPCServer::OnExtendedCommandExecute(boost::bind(&CoreWallet::ExecuteRPC, _1, _2, _3, _4));

    GetMainSignals().ShutdownFinished.connect(boost::bind(&Dealloc));
    GetMainSignals().CreateHelpString.connect(boost::bind(&AppendHelpMessageString, _1, _2));
    GetMainSignals().LoadModules.connect(boost::bind(&LoadAsModule, _1, _2, _3));
    GetMainSignals().ScriptForMining.connect(boost::bind(&GetScriptForMining, _1));
}

void UnregisterSignals()
{
    GetMainSignals().ShutdownFinished.disconnect(boost::bind(&Dealloc));
    GetMainSignals().CreateHelpString.disconnect(boost::bind(&AppendHelpMessageString, _1, _2));
    GetMainSignals().LoadModules.disconnect(boost::bind(&LoadAsModule, _1, _2, _3));
    GetMainSignals().ScriptForMining.disconnect(boost::bind(&GetScriptForMining, _1));
}
};