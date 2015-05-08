// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VALIDATIONINTERFACE_H
#define BITCOIN_VALIDATIONINTERFACE_H

#include <boost/signals2/signal.hpp>

namespace boost
{
    class thread_group;
} // namespace boost

class CBlock;
struct CBlockLocator;
class CTransaction;
class CValidationInterface;
class CValidationState;
class uint256;

// These functions dispatch to one or all registered wallets

/** Register a wallet to receive updates from core */
void RegisterValidationInterface(CValidationInterface* pwalletIn);
/** Unregister a wallet from core */
void UnregisterValidationInterface(CValidationInterface* pwalletIn);
/** Unregister all wallets from core */
void UnregisterAllValidationInterfaces();
/** Push an updated transaction to all registered wallets */
void SyncWithWallets(const CTransaction& tx, const CBlock* pblock = NULL);

class CValidationInterface {
protected:
    virtual void SyncTransaction(const CTransaction &tx, const CBlock *pblock) {}
    virtual void SetBestChain(const CBlockLocator &locator) {}
    virtual void UpdatedTransaction(const uint256 &hash) {}
    virtual void Inventory(const uint256 &hash) {}
    virtual void ResendWalletTransactions(int64_t nBestBlockTime) {}
    virtual void BlockChecked(const CBlock&, const CValidationState&) {}
    friend void ::RegisterValidationInterface(CValidationInterface*);
    friend void ::UnregisterValidationInterface(CValidationInterface*);
    friend void ::UnregisterAllValidationInterfaces();
};

struct CMainSignals {
    /** Notifies listeners of updated transaction data (transaction, and optionally the block it is found in. */
    boost::signals2::signal<void (const CTransaction &, const CBlock *)> SyncTransaction;
    /** Notifies listeners of an updated transaction without new data (for now: a coinbase potentially becoming visible). */
    boost::signals2::signal<void (const uint256 &)> UpdatedTransaction;
    /** Notifies listeners of a new active block chain. */
    boost::signals2::signal<void (const CBlockLocator &)> SetBestChain;
    /** Notifies listeners about an inventory item being seen on the network. */
    boost::signals2::signal<void (const uint256 &)> Inventory;
    /** Tells listeners to broadcast their data. */
    boost::signals2::signal<void (int64_t nBestBlockTime)> Broadcast;
    /** Notifies listeners of a block validation result */
    boost::signals2::signal<void (const CBlock&, const CValidationState&)> BlockChecked;
    
    /** Notifies listeners that the help string gets created where listeners can append characters to the given string */
    boost::signals2::signal<void (std::string& strUsage, bool debugHelp)> CreateHelpString;
    
    /** Notifies listeners that we are in stage of mapping arguments */
    boost::signals2::signal<void (std::string& warningString, std::string& errorString)> ParameterInteraction;

    /** Notifies listeners that we are in stage of initializing the app (datadir, pid file, etc.) */
    boost::signals2::signal<void (std::string& errorString)> AppInitialization;

    /** Notifies listeners that now is the time to log startup/env infos */
    boost::signals2::signal<void ()> AppInitializationLogHead;
    
    /** Notifies listeners that now is the time to log startup/env infos */
    boost::signals2::signal<void (std::string& warningString, std::string& errorString, bool& stopInit)> VerifyIntegrity;
    
    /** Notifies listeners that modules should load now */
    boost::signals2::signal<void (std::string& warningString, std::string& errorString, bool& stopInit)> LoadModules;
    
    /** Notifies listeners that the node has just started */
    boost::signals2::signal<void ()> NodeStarted;
    
    /** Notifies listeners that we have successfully initialized */
    boost::signals2::signal<void (boost::thread_group&)> FinishInitializing;
    
    /** Notifies listeners that we are in shutdown state (RPC stopped) */
    boost::signals2::signal<void ()> ShutdownRPCStopped;
    
    /** Notifies listeners that we are in shutdown state (Node stopped) */
    boost::signals2::signal<void ()> ShutdownNodeStopped;
    
    /** Notifies listeners that we are in shutdown state (Node stopped) */
    boost::signals2::signal<void ()> ShutdownFinished;
};

CMainSignals& GetMainSignals();

#endif // BITCOIN_VALIDATIONINTERFACE_H
