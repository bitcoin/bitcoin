// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COREWALLT_WALLET_H
#define BITCOIN_COREWALLT_WALLET_H

#include "corewallet/coreinterface.h"

#include "corewallet/corewallet_basics.h"
#include "corewallet/corewallet_db.h"
#include "corewallet/corewallet_wtx.h"
#include "corewallet/corewallet_ismine.h"
#include "corewallet/hdkeystore.h"

#include "coincontrol.h"
#include "key.h"
#include "keystore.h"
#include "ui_interface.h"
#include "validationinterface.h"



namespace CoreWallet {

class WalletTx;
class CReserveKey;

static const unsigned int MAX_FREE_TRANSACTION_CREATE_SIZE = 1000;
extern bool bSpendZeroConfChange;
extern bool fSendFreeTransactions;
extern bool fPayAtLeastCustomFee;
extern unsigned int nTxConfirmTarget;
extern CFeeRate payTxFee;
extern CAmount maxTxFee;

class Wallet : public CHDKeyStore, public CValidationInterface{
private:
    //!default coin selection function
    bool SelectCoins(const CAmount& nTargetValue, std::set<std::pair<const WalletTx*,unsigned int> >& setCoinsRet, CAmount& nValueRet, const CCoinControl *coinControl = NULL) const;

    //!wallet tx map
    std::map<uint256, WalletTx> mapWallet;

    //!in memory only map that tracks how many nodes did request a certain hash (mainly tx hash)
    std::map<uint256, int> mapRequestCount;

protected:
    CoreInterface coreInterface; //!instance of a interface between bitcoin-core and the corewallet module
    bool fBroadcastTransactions;

    //!Add a transaction to the wallet if it's relevant
    bool AddToWalletIfInvolvingMe(const CTransaction& tx, const CBlock* pblock, bool fUpdate);

    //!write a wtx to disk
    bool WriteWTXToDisk(const WalletTx &wtx);
    const WalletTx* GetWalletTx(const uint256& hash) const;
    bool IsTrustedWTx(const WalletTx &wtx) const;

public:
    static CFeeRate minTxFee;

    mutable CCriticalSection cs_coreWallet; //main wallet lock
    std::map<CKeyID, CKeyMetadata> mapKeyMetadata; //!map of all generated/derived keys
    std::map<CTxDestination, CAddressBookMetadata> mapAddressBook; //!addressbook with a variant key (CScript/CKeyID)
    std::set<COutPoint> setLockedCoins; //! set for locking coins (in mem only)

    int64_t nTimeFirstKey; //!oldest key timestamp

    //wallet backends
    FileDB *walletPrivateDB;
    FileDB *walletCacheDB;

    //! state: current active hd chain, must be protected over cs_coreWallet
    HDChainID activeHDChain;

    //!wallet constructor, will open wallet and read the database and map everyhing into memory
    Wallet(std::string strWalletFileIn);

    //!adds a hd chain of keys to the wallet
    bool HDSetChainPath(const std::string& chainPath, bool generateMaster, CKeyingMaterial& vSeed, HDChainID& chainId, bool overwrite = false);

    //!gets a child key from the internal or external chain at given index
    bool HDGetChildPubKeyAtIndex(const HDChainID& chainID, CPubKey &pubKeyOut, std::string& newKeysChainpath, unsigned int nIndex, bool internal = false);

    //!get next free child key
    bool HDGetNextChildPubKey(const HDChainID& chainId, CPubKey &pubKeyOut, std::string& newKeysChainpathOut, bool internal = false);

    //!encrypt your master seeds
    bool EncryptHDSeeds(CKeyingMaterial& vMasterKeyIn);

    //!set the active chain of keys
    bool HDSetActiveChainID(const HDChainID& chainID, bool check = true);

    //!set the active chain of keys
    bool HDGetActiveChainID(HDChainID& chainID);

    //!add a key to the keystore (mem only)
    bool InMemAddKey(const CKey& key, const CPubKey &pubkey);
    bool InMemAddKeyMetadata(const CPubKey &pubkey, const CoreWallet::CKeyMetadata &metadata);

     //!add a key to the keystore and store in into the database
    bool AddAndStoreKeyPubKey(const CKey& key, const CPubKey &pubkey);
    bool SetAndStoreAddressBook(const CTxDestination& address, const std::string& purpose);

    //! inform wallet about a new arrived transaction
    void SyncTransaction(const CTransaction& tx, const CBlock* pblock);

    /*
     receiving stack
     */
    //! Add a transaction to the mapWallet (if fOnlyInMemory is false, it will also be stored into the database)
    //no signal are called if fOnlyInMemory == true
    bool AddToWallet(const WalletTx& wtxIn, bool fOnlyInMemory);

    CAmount GetDebit(const CTxIn& txin, const isminefilter& filter) const;
    CAmount GetDebit(const CTransaction& tx, const isminefilter& filter) const;
    CAmount GetCredit(const CTxOut& txout, const isminefilter& filter) const;
    CAmount GetCredit(const CTransaction& tx, const isminefilter& filter) const;

    isminetype IsMine(const CTxIn& txin) const;
    isminetype IsMine(const CTxOut& txout) const;
    bool IsMine(const CTransaction& tx) const;
    bool IsFromMe(const CTransaction& tx, const isminefilter& filter = ISMINE_ALL) const;


    /** Inquire whether this wallet broadcasts transactions. */
    bool GetBroadcastTransactions() const { return fBroadcastTransactions; }
    /** Set whether this wallet broadcasts transactions. */
    void SetBroadcastTransactions(bool broadcast) { fBroadcastTransactions = broadcast; }

    std::set<uint256> GetConflicts(const uint256& txid) const;

    /**
     * Used to keep track of spent outpoints, and
     * detect and report conflicts (double-spends or
     * mutated transactions where the mutant gets mined).
     */
    typedef std::multimap<COutPoint, uint256> TxSpends;
    TxSpends mapTxSpends;
    void AddToSpends(const COutPoint& outpoint, const uint256& wtxid);
    void AddToSpends(const uint256& wtxid);
    bool IsSpent(const uint256& hash, unsigned int n) const;
    void SyncMetaData(std::pair<TxSpends::iterator, TxSpends::iterator>);

    //!sort mapWallet to a ordered multimap
    typedef std::multimap<int64_t, WalletTx*> WtxItems;
    Wallet::WtxItems OrderedTxItems();

    //coin locking functions
    bool IsLockedCoin(uint256 hash, unsigned int n) const;
    void LockCoin(COutPoint& output);
    void UnlockCoin(COutPoint& output);
    void UnlockAllCoins();
    void ListLockedCoins(std::vector<COutPoint>& vOutpts);

    //Balance funtions
    CAmount GetBalance(const enum CREDIT_DEBIT_TYPE &balanceType, const isminefilter& filter) const;

    //Broadcast a wtx over connected core interface
    bool RelayWalletTransaction(const WalletTx &wtx);

    //Coin selection and create transaction functions
    //!get all available coins
    void AvailableCoins(std::vector<COutput>& vCoins, bool fOnlyConfirmed, const CCoinControl *coinControl, bool fIncludeZeroValue = false) const;
    //!select coins for a target value
    bool SelectCoinsMinConf(const CAmount& nTargetValue, int nConfMine, int nConfTheirs, std::vector<COutput> vCoins, std::set<std::pair<const WalletTx*,unsigned int> >& setCoinsRet, CAmount& nValueRet) const;
    //!create a transaction for n recipients, auto-add a change output
    bool CreateTransaction(const std::vector<CRecipient>& vecSend, WalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosRet, std::string& strFailReason, const CCoinControl *coinControl = NULL, bool sign = true);
    bool CommitTransaction(WalletTx& wtxNew, CReserveKey& reservekey);
    CAmount GetMinimumFee(unsigned int nTxBytes, unsigned int nConfirmTarget);


    /* 
     core node interaction
     */
    const CBlockIndex* GetBlockIndex(uint256 blockhash, bool inActiveChain = false) const;
    int GetActiveChainHeight() const;
    int MempoolExists(uint256 hash) const;
    bool AcceptToMemoryPool(const WalletTx &wtx, bool fLimitFree=true, bool fRejectAbsurdFee=true);
    CFeeRate GetMinRelayTxFee();
    unsigned int GetMaxStandardTxSize();



    /*
     mining
     */
    void GetScriptForMining(boost::shared_ptr<CReserveScript> &script);



    /*
     signals
     */
    //!Wallet transaction added, removed or updated.
    boost::signals2::signal<void (Wallet *wallet, const uint256 &hashTx,
                                  ChangeType status)> NotifyTransactionChanged;
};

/** A key allocated from the key pool. */
class CReserveKey : public CReserveScript
{
protected:
    Wallet* pwallet;
    int64_t nIndex;
    CPubKey vchPubKey;
public:
    CReserveKey(Wallet* pwalletIn)
    {
        nIndex = -1;
        pwallet = pwalletIn;
    }

    ~CReserveKey()
    {
        ReturnKey();
    }

    void ReturnKey();
    virtual bool GetReservedKey(CPubKey &pubkey);
    void KeepKey();
    void KeepScript() { KeepKey(); }
};

class CHDReserveKey : public CReserveKey
{
protected:
    CPubKey vchPubKey;
    HDChainID chainID;
public:
    CHDReserveKey(Wallet* pwalletIn) : CReserveKey(pwalletIn)
    {
        pwalletIn->HDGetActiveChainID(chainID);
    }

    ~CHDReserveKey()
    {
        ReturnKey();
    }

    void ReturnKey();
    bool GetReservedKey(CPubKey &pubkey);
    void KeepKey();
};

}

#endif // BITCOIN_COREWALLT_WALLET_H
