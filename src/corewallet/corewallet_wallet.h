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
    bool SelectCoins(const CAmount& nTargetValue, std::set<std::pair<const WalletTx*,unsigned int> >& setCoinsRet, CAmount& nValueRet, const CCoinControl *coinControl = NULL) const;
public:
    static CFeeRate minTxFee;
    CoreInterface coreInterface;

    mutable CCriticalSection cs_coreWallet;
    std::map<CKeyID, CKeyMetadata> mapKeyMetadata;
    std::map<CTxDestination, CAddressBookMetadata> mapAddressBook;
    std::map<uint256, int> mapRequestCount;
    std::map<uint256, WalletTx> mapWallet;
    std::set<COutPoint> setLockedCoins;
    int64_t nTimeFirstKey;
    FileDB *walletPrivateDB;
    FileDB *walletCacheDB;

    //! state: current active hd chain, must be protected over cs_coreWallet
    HDChainID activeHDChain;

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
    
    bool AddKeyPubKey(const CKey& key, const CPubKey &pubkey);
    bool LoadKeyMetadata(const CPubKey &pubkey, const CoreWallet::CKeyMetadata &metadata);
    bool LoadKey(const CKey& key, const CPubKey &pubkey);
    bool SetAddressBook(const CTxDestination& address, const std::string& purpose);


    void SyncTransaction(const CTransaction& tx, const CBlock* pblock);

    //receiving stack
    bool AddToWallet(const WalletTx& wtxIn, bool fFromLoadWallet);
    bool AddToWalletIfInvolvingMe(const CTransaction& tx, const CBlock* pblock, bool fUpdate);
    isminetype IsMine(const CTxIn& txin) const;
    CAmount GetDebit(const CTxIn& txin, const isminefilter& filter) const;
    isminetype IsMine(const CTxOut& txout) const;
    CAmount GetCredit(const CTxOut& txout, const isminefilter& filter) const;
    bool IsMine(const CTransaction& tx) const;
    /** should probably be renamed to IsRelevantToMe */
    bool IsFromMe(const CTransaction& tx) const;
    CAmount GetDebit(const CTransaction& tx, const isminefilter& filter) const;
    CAmount GetCredit(const CTransaction& tx, const isminefilter& filter) const;

    bool fBroadcastTransactions;
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

    const WalletTx* GetWalletTx(const uint256& hash) const;

    typedef std::pair<WalletTx*, std::string > TxPair;
    typedef std::multimap<int64_t, TxPair > TxItems;
    Wallet::TxItems OrderedTxItems();

    bool WriteWTXToDisk(const WalletTx &wtx);

    bool IsTrustedWTx(const WalletTx &wtx) const;
    void AvailableCoins(std::vector<COutput>& vCoins, bool fOnlyConfirmed, const CCoinControl *coinControl, bool fIncludeZeroValue = false) const;

    bool IsLockedCoin(uint256 hash, unsigned int n) const;
    void LockCoin(COutPoint& output);
    void UnlockCoin(COutPoint& output);
    void UnlockAllCoins();
    void ListLockedCoins(std::vector<COutPoint>& vOutpts);

    //Balance funtions // needs refactor
    CAmount GetBalance(const enum CREDIT_DEBIT_TYPE &balanceType, const isminefilter& filter) const;

    bool RelayWalletTransaction(const WalletTx &wtx);

    bool SelectCoinsMinConf(const CAmount& nTargetValue, int nConfMine, int nConfTheirs, std::vector<COutput> vCoins, std::set<std::pair<const WalletTx*,unsigned int> >& setCoinsRet, CAmount& nValueRet) const;
    bool CreateTransaction(const std::vector<CRecipient>& vecSend, WalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosRet, std::string& strFailReason, const CCoinControl *coinControl = NULL, bool sign = true);
    bool CommitTransaction(WalletTx& wtxNew, CReserveKey& reservekey);
    CAmount GetMinimumFee(unsigned int nTxBytes, unsigned int nConfirmTarget);

    //core node interaction
    const CBlockIndex* GetBlockIndex(uint256 blockhash, bool inActiveChain = false) const;
    int GetActiveChainHeight() const;
    int MempoolExists(uint256 hash) const;
    bool AcceptToMemoryPool(const WalletTx &wtx, bool fLimitFree=true, bool fRejectAbsurdFee=true);
    CFeeRate GetMinRelayTxFee();
    unsigned int GetMaxStandardTxSize();

    //mining
    void GetScriptForMining(boost::shared_ptr<CReserveScript> &script);

    /**
     * Wallet transaction added, removed or updated.
     * @note called with lock cs_wallet held.
     */
    boost::signals2::signal<void (Wallet *wallet, const uint256 &hashTx,
                                  ChangeType status)> NotifyTransactionChanged;
};

// WalletModel: a wallet metadata class
class WalletModel
{
public:
    static const int CURRENT_VERSION=1;
    int nVersion;
    
    Wallet* pWallet; //no persistance
    std::string walletID; //only A-Za-z0-9._-
    std::string strWalletFilename;
    int64_t nCreateTime; // 0 means unknown
    
    WalletModel()
    {
        SetNull();
    }
    
    WalletModel(const std::string& filenameIn, Wallet *pWalletIn)
    {
        SetNull();
        
        strWalletFilename = filenameIn;
        pWallet = pWalletIn;
    }
    
    void SetNull()
    {
        nVersion = CURRENT_VERSION;
        nCreateTime = 0;
        pWallet = NULL;
    }
    
    ADD_SERIALIZE_METHODS;
    
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(nCreateTime);
        READWRITE(strWalletFilename);
    }
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
