// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PARTICL_WALLET_HDWALLET_H
#define PARTICL_WALLET_HDWALLET_H

#include "wallet/wallet.h"
#include "wallet/hdwalletdb.h"
#include "wallet/rpchdwallet.h"
#include "base58.h"

#include "key/extkey.h"
#include "key/stealth.h"

#include "../miner.h"

typedef std::map<CKeyID, CStealthKeyMetadata> StealthKeyMetaMap;
typedef std::map<CKeyID, CExtKeyAccount*> ExtKeyAccountMap;
typedef std::map<CKeyID, CStoredExtKey*> ExtKeyMap;

typedef std::map<uint256, CWalletTx> WalletTxMap;

typedef std::map<uint256, CTransactionRecord> MapRecords_t;

typedef std::multimap<int64_t, std::map<uint256, CTransactionRecord>::iterator> RtxOrdered_t;



enum OutputRecordFlags
{
    ORF_OWNED        = (1 << 0),
    ORF_FROM         = (1 << 1),
    ORF_CHANGE       = (1 << 2),
};

enum OutputRecordAddressTypes
{
    ORA_EXTKEY       = 1,
    ORA_STEALTH      = 2,
    ORA_STANDARD     = 3,
};

class COutputRecord
{
public:
    COutputRecord() : nType(0), nFlags(0), nValue(-1) {};
    uint8_t nType;
    uint8_t nFlags;
    int n;
    CAmount nValue;
    CScript scriptPubKey;
    std::string sNarration;
    
    /*
    vPath 0 - ORA_EXTKEY
        1 - index to m
        2... path
    
    vPath 0 - ORA_STEALTH
        [1, 21] stealthkeyid
        [22, 55] pubkey (if not using ephemkey)
    
    vPath 0 - ORA_STANDARD
        [1, 34] pubkey
    */
    std::vector<uint8_t> vPath; // index to m is stored in first entry
    
    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action)
    {
        READWRITE(nType);
        READWRITE(nFlags);
        READWRITE(n);
        READWRITE(nValue);
        READWRITE(*(CScriptBase*)(&scriptPubKey));
        READWRITE(sNarration);
        READWRITE(vPath);
    }
};

enum RTxAddonValueTypes
{
    RTXVT_EPHEM_PATH            = 1, // path ephemeral keys are derived from packed 4bytes no separators
    
    RTXVT_REPLACES_TXID         = 2,
    RTXVT_REPLACED_BY_TXID      = 3,
    
    RTXVT_COMMENT               = 4,
    RTXVT_TO                    = 5,
    
    /*
    RTXVT_STEALTH_KEYID     = 2,
    RTXVT_STEALTH_KEYID_N   = 3, // n0:pk0:n1:pk1:...
    */
    
};

typedef std::map<uint8_t, std::vector<uint8_t> > mapRTxValue_t;
class CTransactionRecord
{
// stored by uint256 txnHash;
public:
    CTransactionRecord()
    {
        
    };
    
    uint256 blockHash;
    int nIndex;
    int64_t nBlockTime;
    int64_t nTimeReceived;
    CAmount nFee;
    mapRTxValue_t mapValue;
    
    std::vector<COutputRecord> vout;
    
    int InsertOutput(COutputRecord &r)
    {
        for (size_t i = 0; i < vout.size(); ++i)
        {
            if (vout[i].n == r.n)
                return 0; // duplicate
            
            if (vout[i].n < r.n)
                continue;
            
            vout.insert(vout.begin() + i, r);
            return 1;
        };
        vout.push_back(r);
        return 1;
    }
    
    COutputRecord *GetOutput(int n) 
    {
        // vout is always in order by asc n
        for (auto &r : vout)
        {
            if (r.n > n)
                return NULL;
            if (r.n == n)
                return &r;
        }
        return NULL;
    }
    
    const COutputRecord *GetOutput(int n) const 
    {
        // vout is always in order by asc n
        for (auto &r : vout)
        {
            if (r.n > n)
                return NULL;
            if (r.n == n)
                return &r;
        }
        return NULL;
    }
    
    void SetMerkleBranch(const uint256 &blockHash_, int posInBlock)
    {
        blockHash = blockHash_;
        nIndex = posInBlock;
    };
    
    bool IsAbandoned() const { return (blockHash == ABANDON_HASH); }
    
    mutable uint32_t nCacheFlags;

    bool InMempool() const;
    bool IsTrusted() const;

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action)
    {
        READWRITE(blockHash);
        READWRITE(nIndex);
        READWRITE(nBlockTime);
        READWRITE(nTimeReceived);
        READWRITE(mapValue);
        READWRITE(nFee);
        READWRITE(vout);
    }
};


class CTempRecipient
{
public:
    CTempRecipient() : nType(0), nAmount(0), fSubtractFeeFromAmount(false) {SetNull();};
    CTempRecipient(CAmount nAmount_, bool fSubtractFeeFromAmount_, CScript scriptPubKey_)
        : nAmount(nAmount_), fSubtractFeeFromAmount(fSubtractFeeFromAmount_), scriptPubKey(scriptPubKey_) {SetNull();};
    
    void SetNull()
    {
        fChange = false;
        nChildKey = 0;
    }
    
    uint8_t nType;
    CAmount nAmount;
    bool fSubtractFeeFromAmount;
    CTxDestination address;
    CScript scriptPubKey;
    std::vector<uint8_t> vData;
    std::vector<uint8_t> vBlind;
    std::vector<uint8_t> vRangeproof;
    secp256k1_pedersen_commitment commitment;
    
    // TODO: range proof parameters
    
    CKey sEphem;
    CPubKey pkTo;
    int n;
    std::string sNarration;
    bool fChange;
    uint32_t nChildKey; // update later
};


class COutputR
{
public:
    COutputR() {};
    
    COutputR(const uint256 &txhash_, MapRecords_t::const_iterator rtx_, int i_, int nDepth_)
        : txhash(txhash_), rtx(rtx_), i(i_), nDepth(nDepth_) {};
    
    uint256 txhash;
    //const CTransactionRecord *rtx;
    MapRecords_t::const_iterator rtx;
    int i;
    int nDepth;
};


class CStoredTransaction
{
public:
    CTransactionRef tx;
    //std::vector<uint8_t> vBlinds;
    std::vector<std::pair<int, uint256> > vBlinds;
    
    bool InsertBlind(int n, const uint8_t *p)
    {
        for (auto &bp : vBlinds)
        {
            if (bp.first == n)
            {
                memcpy(bp.second.begin(), p, 32);
                return true;
            };
        };
        uint256 insert;
        memcpy(insert.begin(), p, 32);
        vBlinds.push_back(std::make_pair(n, insert));
        return true;
    };
    
    bool GetBlind(int n, uint8_t *p) const
    {
        for (auto &bp : vBlinds)
        {
            if (bp.first == n)
            {
                memcpy(p, bp.second.begin(), 32);
                return true;
            };
        };
        return false;
    };
    
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(tx);
        READWRITE(vBlinds);
    }
};

class CHDWallet : public CWallet
{
public:
    CHDWallet(const std::string &strWalletFileIn)
    {
        strWalletFile = strWalletFileIn;
        SetNull();
        fFileBacked = true;
        nReserveBalance = 0;
        
        pEKMaster = NULL;
    };
    
    ~CHDWallet()
    {
        Finalise();
    };
    
    int Finalise();
    int FreeExtKeyMaps();
    
    static bool InitLoadWallet();
    
    bool LoadAddressBook(CHDWalletDB *pwdb);
    
    bool LoadVoteTokens(CHDWalletDB *pwdb);
    bool GetVote(int nHeight, uint32_t &token);
    
    bool LoadTxRecords(CHDWalletDB *pwdb);
    
    bool EncryptWallet(const SecureString &strWalletPassphrase);
    bool Lock();
    bool Unlock(const SecureString &strWalletPassphrase);
    
    
    bool HaveAddress(const CBitcoinAddress &address);
    bool HaveKey(const CKeyID &address, CEKAKey &ak, CExtKeyAccount *pa) const;
    bool HaveKey(const CKeyID &address) const;
    
    bool HaveExtKey(const CKeyID &address) const;
    
    bool GetKey(const CKeyID &address, CKey &keyOut) const;
    
    bool GetPubKey(const CKeyID &address, CPubKey &pkOut) const;
    
    bool HaveStealthAddress(const CStealthAddress &sxAddr) const;
    
    bool ImportStealthAddress(const CStealthAddress &sxAddr, const CKey &skSpend);
    
    bool AddressBookChangedNotify(const CTxDestination &address, ChangeType nMode);
    bool SetAddressBook(CHDWalletDB *pwdb, const CTxDestination &address, const std::string &strName,
        const std::string &purpose, const std::vector<uint32_t> &vPath, bool fNotifyChanged=true);
    bool SetAddressBook(const CTxDestination &address, const std::string &strName, const std::string &purpose);
    bool DelAddressBook(const CTxDestination &address);
    
    
    isminetype IsMine(const CScript &scriptPubKey, CKeyID &keyID,
        CEKAKey &ak, CExtKeyAccount *pa, bool &isInvalid, SigVersion = SIGVERSION_BASE);
    
    isminetype IsMine(const CTxOutBase *txout) const;
    bool IsMine(const CTransaction& tx) const;
    bool IsFromMe(const CTransaction& tx) const;
    
    /**
     * Returns amount of debit if the input matches the
     * filter, otherwise returns 0
     */
    CAmount GetDebit(const CTxIn& txin, const isminefilter& filter) const;
    CAmount GetDebit(const CTransaction& tx, const isminefilter& filter) const;
    
    CAmount GetCredit(const CTxOutBase *txout, const isminefilter &filter) const;
    CAmount GetCredit(const CTransaction &tx, const isminefilter &filter) const;
    
    int GetDepthInMainChain(const uint256 &blockhash) const;
    bool InMempool(const uint256 &hash) const;
    bool IsTrusted(const uint256 &hash, const uint256 &blockhash) const;
    
    CAmount GetBalance() const;
    CAmount GetUnconfirmedBalance() const;
    CAmount GetBlindBalance();
    CAmount GetAnonBalance();
    CAmount GetStaked();
    
    
    bool IsChange(const CTxOutBase *txout) const;
    
    int GetChangeAddress(CPubKey &pk);
    
    int ExpandTempRecipients(std::vector<CTempRecipient> &vecSend, CStoredExtKey *pc, std::string &sError);
    
    /** Update wallet after successfull transaction */
    int PostProcessTempRecipients(std::vector<CTempRecipient> &vecSend);
    
    int AddStandardInputs(CWalletTx &wtx, CTransactionRecord &rtx,
        std::vector<CTempRecipient> &vecSend,
        CExtKeyAccount *sea, CStoredExtKey *pc,
        bool sign, std::string &sError);
    int AddStandardInputs(CWalletTx &wtx, CTransactionRecord &rtx,
        std::vector<CTempRecipient> &vecSend, bool sign, std::string &sError);
    
    int AddBlindedInputs(CWalletTx &wtx, CTransactionRecord &rtx,
        std::vector<CTempRecipient> &vecSend,
        CExtKeyAccount *sea, CStoredExtKey *pc,
        bool sign, std::string &sError);
    int AddBlindedInputs(CWalletTx &wtx, CTransactionRecord &rtx,
        std::vector<CTempRecipient> &vecSend, bool sign, std::string &sError);
    
    
    
    bool LoadToWallet(const CWalletTx& wtxIn);
    bool LoadToWallet(const uint256 &hash, const CTransactionRecord &rtx);
    
    /** Remove txn from mapwallet and TxSpends */
    int UnloadTransaction(const uint256 &hash);
    
    int GetDefaultConfidentialChain(CHDWalletDB *pwdb, CExtKeyAccount *&sea, CStoredExtKey *&pc);
    
    int ExtKeyNew32(CExtKey &out);
    int ExtKeyNew32(CExtKey &out, const char *sPassPhrase, int32_t nHash, const char *sSeed);
    int ExtKeyNew32(CExtKey &out, uint8_t *data, uint32_t lenData);
    
    int ExtKeyImportLoose(CHDWalletDB *pwdb, CStoredExtKey &sekIn, CKeyID &idDerived, bool fBip44, bool fSaveBip44);
    int ExtKeyImportAccount(CHDWalletDB *pwdb, CStoredExtKey &sekIn, int64_t nTimeStartScan, const std::string &sLabel);
    
    int ExtKeySetMaster(CHDWalletDB *pwdb, CKeyID &idMaster); // set master to existing key, remove master key tag from old key if exists
    int ExtKeyNewMaster(CHDWalletDB *pwdb, CKeyID &idMaster, bool fAutoGenerated = false); // make and save new root key to wallet
    
    int ExtKeyCreateAccount(CStoredExtKey *ekAccount, CKeyID &idMaster, CExtKeyAccount &ekaOut, const std::string &sLabel);
    int ExtKeyDeriveNewAccount(CHDWalletDB *pwdb, CExtKeyAccount *sea, const std::string &sLabel, const std::string &sPath=""); // derive a new account from the master key and save to wallet
    int ExtKeySetDefaultAccount(CHDWalletDB *pwdb, CKeyID &idNewDefault);
    
    int ExtKeyEncrypt(CStoredExtKey *sek, const CKeyingMaterial &vMKey, bool fLockKey);
    int ExtKeyEncrypt(CExtKeyAccount *sea, const CKeyingMaterial &vMKey, bool fLockKey);
    int ExtKeyEncryptAll(CHDWalletDB *pwdb, const CKeyingMaterial &vMKey);
    int ExtKeyLock();
    
    int ExtKeyUnlock(CExtKeyAccount *sea);
    int ExtKeyUnlock(CExtKeyAccount *sea, const CKeyingMaterial &vMKey);
    int ExtKeyUnlock(CStoredExtKey *sek);
    int ExtKeyUnlock(CStoredExtKey *sek, const CKeyingMaterial &vMKey);
    int ExtKeyUnlock(const CKeyingMaterial &vMKey);
    
    int ExtKeyCreateInitial(CHDWalletDB *pwdb);
    int ExtKeyLoadMaster();
    int ExtKeyLoadAccounts();
    
    int ExtKeySaveAccountToDB(CHDWalletDB *pwdb, CKeyID &idAccount, CExtKeyAccount *sea);
    int ExtKeyAddAccountToMaps(CKeyID &idAccount, CExtKeyAccount *sea);
    int ExtKeyRemoveAccountFromMapsAndFree(CExtKeyAccount *sea);
    int ExtKeyLoadAccountPacks();

    int ExtKeyAppendToPack(CHDWalletDB *pwdb, CExtKeyAccount *sea, const CKeyID &idKey, CEKAKey &ak, bool &fUpdateAcc) const;
    int ExtKeyAppendToPack(CHDWalletDB *pwdb, CExtKeyAccount *sea, const CKeyID &idKey, CEKASCKey &asck, bool &fUpdateAcc) const;

    int ExtKeySaveKey(CHDWalletDB *pwdb, CExtKeyAccount *sea, const CKeyID &keyId, CEKAKey &ak) const;
    int ExtKeySaveKey(CExtKeyAccount *sea, const CKeyID &keyId, CEKAKey &ak) const;

    int ExtKeySaveKey(CHDWalletDB *pwdb, CExtKeyAccount *sea, const CKeyID &keyId, CEKASCKey &asck) const;
    int ExtKeySaveKey(CExtKeyAccount *sea, const CKeyID &keyId, CEKASCKey &asck) const;

    int ExtKeyUpdateStealthAddress(CHDWalletDB *pwdb, CExtKeyAccount *sea, CKeyID &sxId, std::string &sLabel);
    
    /**
     * Create an index db record for idKey
     */
    int ExtKeyNewIndex(CHDWalletDB *pwdb, const CKeyID &idKey, uint32_t &index);
    int ExtKeyGetIndex(CHDWalletDB *pwdb, CExtKeyAccount *sea, uint32_t &index, bool &fUpdate);

    int NewKeyFromAccount(CHDWalletDB *pwdb, const CKeyID &idAccount, CPubKey &pkOut, bool fInternal, bool fHardened, const char *plabel = NULL);
    int NewKeyFromAccount(CPubKey &pkOut, bool fInternal=false, bool fHardened=false, const char *plabel = NULL); // wrapper - use default account

    int NewStealthKeyFromAccount(CHDWalletDB *pwdb, const CKeyID &idAccount, std::string &sLabel, CEKAStealthKey &akStealthOut, uint32_t nPrefixBits, const char *pPrefix);
    int NewStealthKeyFromAccount(std::string &sLabel, CEKAStealthKey &akStealthOut, uint32_t nPrefixBits, const char *pPrefix); // wrapper - use default account

    int NewExtKeyFromAccount(CHDWalletDB *pwdb, const CKeyID &idAccount, std::string &sLabel, CStoredExtKey *sekOut, const char *plabel = NULL, uint32_t *childNo=NULL);
    int NewExtKeyFromAccount(std::string &sLabel, CStoredExtKey *sekOut, const char *plabel = NULL, uint32_t *childNo=NULL); // wrapper - use default account

    int ExtKeyGetDestination(const CExtKeyPair &ek, CPubKey &pkDest, uint32_t &nKey);
    int ExtKeyUpdateLooseKey(const CExtKeyPair &ek, uint32_t nKey, bool fAddToAddressBook);
    
    int ScanChainFromTime(int64_t nTimeStartScan);
    int ScanChainFromHeight(int nHeight);
    
    bool CreateTransaction(const std::vector<CRecipient>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
                           std::string& strFailReason, const CCoinControl *coinControl = NULL, bool sign = true);
    bool CommitTransaction(CWalletTx& wtxNew, CReserveKey& reservekey, CConnman* connman, CValidationState& state);
    bool CommitTransaction(CWalletTx &wtxNew, CTransactionRecord &rtx,
        CReserveKey &reservekey, CConnman *connman, CValidationState &state);
    
    int LoadStealthAddresses();
    bool UpdateStealthAddressIndex(const CKeyID &idK, const CStealthAddressIndexed &sxi, uint32_t &id); // Get stealth index or create new index if none found
    bool GetStealthLinked(const CKeyID &idK, CStealthAddress &sx);
    bool ProcessLockedStealthOutputs();
    bool ProcessStealthOutput(const CTxDestination &address,
        std::vector<uint8_t> &vchEphemPK, uint32_t prefix, bool fHavePrefix, CKey &sShared);
    
    int CheckForStealthAndNarration(const CTxOutBase *pb, const CTxOutData *pdata, std::string &sNarr);
    bool FindStealthTransactions(const CTransaction &tx, mapValue_t &mapNarr);
    
    bool AddToWalletIfInvolvingMe(const CTransaction& tx, const CBlockIndex* pIndex, int posInBlock, bool fUpdate);
    
    const CWalletTx *GetWalletTx(const uint256& hash) const;
    CWalletTx *GetWalletTx(const uint256& hash);
    
    int InsertTempTxn(const uint256 &txid, const uint256 &blockHash, int nIndex) const;
    
    int OwnStandardOut(const CTxOutStandard *pout, const CTxOutData *pdata, COutputRecord &rout);
    int OwnBlindOut(const CTxOutCT *pout, const CStoredExtKey *pc, uint32_t &nLastChild,
        COutputRecord &rout, CStoredTransaction &stx);
    int OwnAnonOut(const CTxOutRingCT *pout, const CStoredExtKey *pc, uint32_t &nLastChild,
        COutputRecord &rout, CStoredTransaction &stx);
    
    bool AddToRecord(CTransactionRecord &rtxIn, const CTransaction &tx,
        const CBlockIndex *pIndex, int posInBlock, bool fFlushOnClose=true);
    
    
    /**
     * populate vCoins with vector of available COutputs.
     */
    void AvailableCoins(std::vector<COutput>& vCoins, bool fOnlyConfirmed=true, const CCoinControl *coinControl = NULL, bool fIncludeZeroValue=false) const;
    bool SelectCoins(const std::vector<COutput>& vAvailableCoins, const CAmount& nTargetValue, std::set<std::pair<const CWalletTx*,unsigned int> >& setCoinsRet, CAmount& nValueRet, const CCoinControl *coinControl = NULL) const;
    
    
    void AvailableBlindedCoins(std::vector<COutputR>& vCoins, bool fOnlyConfirmed=true, const CCoinControl *coinControl = NULL, bool fIncludeZeroValue=false) const;
    bool SelectBlindedCoins(const std::vector<COutputR>& vAvailableCoins, const CAmount& nTargetValue, std::vector<std::pair<MapRecords_t::const_iterator,unsigned int> >& setCoinsRet, CAmount& nValueRet, const CCoinControl *coinControl = NULL) const;
    
    bool SelectCoinsMinConf(const CAmount& nTargetValue, int nConfMine, int nConfTheirs, uint64_t nMaxAncestors, std::vector<COutputR> vCoins, std::vector<std::pair<MapRecords_t::const_iterator,unsigned int> >& setCoinsRet, CAmount& nValueRet) const;
    
    bool IsSpent(const uint256& hash, unsigned int n) const;
    
    void SyncMetaData(std::pair<TxSpends::iterator, TxSpends::iterator>);
    
    
    void AvailableCoinsForStaking(std::vector<COutput> &vCoins, int64_t nTime, int nHeight);
    bool SelectCoinsForStaking(int64_t nTargetValue, int64_t nTime, int nHeight, std::set<std::pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet);
    bool CreateCoinStake(unsigned int nBits, int64_t nTime, int nBlockHeight, int64_t nFees, CMutableTransaction &txNew, CKey &key);
    bool SignBlock(CBlockTemplate *pblocktemplate, int nHeight, int64_t nSearchTime);
    
    int64_t nLastCoinStakeSearchTime = 0;
    uint32_t nStealth, nFoundStealth; // for reporting, zero before use
    int64_t nReserveBalance;
    int deepestTxnDepth = 0; // for stake mining
    
    std::set<CStealthAddress> stealthAddresses;
    
    CStoredExtKey *pEKMaster;
    CKeyID idDefaultAccount;
    ExtKeyAccountMap mapExtAccounts;
    ExtKeyMap mapExtKeys;
    
    mutable std::map<uint256, CWalletTx> mapTempWallet;
    
    MapRecords_t mapRecords;
    
    RtxOrdered_t rtxOrdered;
    
    std::vector<CVoteToken> vVoteTokens;
    
};

int ToStealthRecipient(CStealthAddress &sx, CAmount nValue, bool fSubtractFeeFromAmount,
    std::vector<CRecipient> &vecSend, std::string &sNarr, std::string &strError);

class LoopExtKeyCallback
{
public:
    // NOTE: the key and account instances passed to Process are temporary
    virtual int ProcessKey(CKeyID &id, CStoredExtKey &sek) {return 1;};
    virtual int ProcessAccount(CKeyID &id, CExtKeyAccount &sek) {return 1;};
};

int LoopExtKeysInDB(bool fInactive, bool fInAccount, LoopExtKeyCallback &callback);
int LoopExtAccountsInDB(bool fInactive, LoopExtKeyCallback &callback);

#endif // PARTICL_WALLET_HDWALLET_H
