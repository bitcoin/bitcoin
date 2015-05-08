// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "corewallet/corewallet.h"
#include "corewallet/corewallet_db.h"
#include "rpcserver.h"
#include "ui_interface.h"
#include "util.h"
#include "validationinterface.h"

#include <string>

#include <boost/foreach.hpp>

namespace CoreWallet {

const static std::string DEFAULT_WALLET_FILE = "wallet.wal";
static std::map<std::string, WalletModel*> mapWallets;
static FileDB *walletsListDB;
    
//implemented in corewallet_rpc.cpp
extern void ExecuteRPC(const std::string& strMethod, const json_spirit::Array& params, json_spirit::Value& result, bool& accept);

CoreWallet::Wallet* pCoreWallet = NULL;


CoreWallet::Wallet* GetWallet()
{
    return pCoreWallet;
}
    
void AppendHelpMessageString(std::string& strUsage, bool debugHelp)
{
    if (debugHelp)
        return;
    
    strUsage += HelpMessageGroup(_("CoreWallet options:"));
    strUsage += HelpMessageOpt("-disablecorewallet", _("Do not load the wallet and disable wallet RPC calls"));
}

void LoadAsModule(std::string& warningString, std::string& errorString, bool& stopInit)
{
    pCoreWallet = new Wallet(DEFAULT_WALLET_FILE);
}
    
void Dealloc()
{
    delete walletsListDB;
    walletsListDB = NULL;
}
    
void RegisterSignals()
{
    AddJSONRPCURISchema("/corewallet");
    RPCServer::OnExtendedCommandExecute(boost::bind(&CoreWallet::ExecuteRPC, _1, _2, _3, _4));
    
    GetMainSignals().ShutdownFinished.connect(boost::bind(&Dealloc));
    GetMainSignals().CreateHelpString.connect(boost::bind(&AppendHelpMessageString, _1, _2));
    GetMainSignals().LoadModules.connect(boost::bind(&LoadAsModule, _1, _2, _3));
}

void UnregisterSignals()
{
    GetMainSignals().ShutdownFinished.disconnect(boost::bind(&Dealloc));
    GetMainSignals().CreateHelpString.disconnect(boost::bind(&AppendHelpMessageString, _1, _2));
    GetMainSignals().LoadModules.disconnect(boost::bind(&LoadAsModule, _1, _2, _3));
}
};