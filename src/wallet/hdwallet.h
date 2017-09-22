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

typedef std::map<uint256, CWalletTx> MapWallet_t;
typedef std::map<uint256, CTransactionRecord> MapRecords_t;

typedef std::multimap<int64_t, std::map<uint256, CTransactionRecord>::iterator> RtxOrdered_t;

class UniValue;

const uint16_t PLACEHOLDER_N = 0xFFFF;
enum OutputRecordFlags
{
    ORF_OWNED        = (1 << 0),
    ORF_FROM         = (1 << 1),
    ORF_CHANGE       = (1 << 2),
    ORF_SPENT        = (1 << 3),
    ORF_LOCKED       = (1 << 4), // Needs wallet to be unlocked for further processing
    ORF_STAKEONLY    = (1 << 5),
    
    ORF_BLIND_IN     = (1 << 14),
    ORF_ANON_IN      = (1 << 15),
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
    COutputRecord() : nType(0), nFlags(0), n(0), nValue(-1) {};
    uint8_t nType;
    uint8_t nFlags;
    uint16_t n;
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
    };
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
// Stored by uint256 txnHash;
public:
    CTransactionRecord() :
        nFlags(0), nIndex(0), nBlockTime(0) , nTimeReceived(0) , nFee(0) {};
    
    // Conflicted state is marked by set blockHash and nIndex -1
    uint256 blockHash;
    int16_t nFlags;
    int16_t nIndex;
    
    int64_t nBlockTime;
    int64_t nTimeReceived;
    CAmount nFee;
    mapRTxValue_t mapValue;
    
    std::vector<COutPoint> vin;
    std::vector<COutputRecord> vout;
    
    int InsertOutput(COutputRecord &r);
    bool EraseOutput(uint16_t n);
    
    COutputRecord *GetOutput(int n);
    const COutputRecord *GetOutput(int n) const;
    
    const COutputRecord *GetChangeOutput() const;
    
    void SetMerkleBranch(const uint256 &blockHash_, int posInBlock)
    {
        blockHash = blockHash_;
        nIndex = posInBlock;
    };
    
    bool IsAbandoned() const { return (blockHash == ABANDON_HASH); }
    bool HashUnset() const { return (blockHash.IsNull() || blockHash == ABANDON_HASH); }
    
    void SetAbandoned()
    {
        blockHash = ABANDON_HASH;
    };
    
    int64_t GetTxTime() const
    {
        if (HashUnset() || nIndex < 0)
            return nTimeReceived;
        return std::min(nTimeReceived, nBlockTime);
    };
    
    bool HaveChange() const
    {
        for (const auto &r : vout)
            if (r.nFlags & ORF_CHANGE)
                return true;
        return false;
    };
    
    CAmount TotalOutput()
    {
        CAmount nTotal = 0;
        for (auto &r : vout)
            nTotal += r.nValue;
        return nTotal;
    };
    
    mutable uint32_t nCacheFlags;

    bool InMempool() const;
    bool IsTrusted() const;
    
    bool IsCoinBase() const {return false;}; 
    bool IsCoinStake() const {return false;};

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action)
    {
        READWRITE(blockHash);
        READWRITE(nFlags);
        READWRITE(nIndex);
        READWRITE(nBlockTime);
        READWRITE(nTimeReceived);
        READWRITE(mapValue);
        READWRITE(nFee);
        READWRITE(vin);
        READWRITE(vout);
    };
};


class CTempRecipient
{
public:
    CTempRecipient() : nType(0), nAmount(0), nAmountSelected(0), fSubtractFeeFromAmount(false) {SetNull();};
    CTempRecipient(CAmount nAmount_, bool fSubtractFeeFromAmount_, CScript scriptPubKey_)
        : nAmount(nAmount_), nAmountSelected(nAmount_), fSubtractFeeFromAmount(fSubtractFeeFromAmount_), scriptPubKey(scriptPubKey_) {SetNull();};
    
    void SetNull()
    {
        fScriptSet = false;
        fChange = false;
        nChildKey = 0;
        nChildKeyColdStaking = 0;
        nStealthPrefix = 0;
        fSplitBlindOutput = false;
        fExemptFeeSub = false;
    };
    
    void SetAmount(CAmount nValue)
    {
        nAmount = nValue;
        nAmountSelected = nValue;
    };
    
    bool ApplySubFee(CAmount nFee, size_t nSubtractFeeFromAmount, bool &fFirst);
    
    uint8_t nType;
    CAmount nAmount;            // If fSubtractFeeFromAmount, nAmount = nAmountSelected - feeForOutput
    CAmount nAmountSelected; 
    bool fSubtractFeeFromAmount;
    bool fSplitBlindOutput;
    bool fExemptFeeSub;         // Value too low to sub fee when blinded value split into two outputs
    CTxDestination address;
    CTxDestination addressColdStaking;
    CScript scriptPubKey;
    std::vector<uint8_t> vData;
    std::vector<uint8_t> vBlind;
    std::vector<uint8_t> vRangeproof;
    secp256k1_pedersen_commitment commitment;
    
    // TODO: range proof parameters, try to keep similar for fee
    
    
    CKey sEphem;
    CPubKey pkTo;
    int n;
    std::string sNarration;
    bool fScriptSet;
    bool fChange;
    uint32_t nChildKey; // update later
    uint32_t nChildKeyColdStaking; // update later
    uint32_t nStealthPrefix;
};


class COutputR
{
public:
    COutputR() {};
    
    COutputR(const uint256 &txhash_, MapRecords_t::const_iterator rtx_, int i_, int nDepth_, bool fSafe_, bool fMature_)
        : txhash(txhash_), rtx(rtx_), i(i_), nDepth(nDepth_), fSafe(fSafe_), fMature(fMature_) {};
    
    uint256 txhash;
    MapRecords_t::const_iterator rtx;
    int i;
    int nDepth;
    bool fSpendable;
    bool fSolvable;
    bool fSafe;
    bool fMature;
};


class CStoredTransaction
{
public:
    CTransactionRef tx;
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
    };
};

class CHDWalletBalances
{
public:
    void Clear()
    {
        nPart = 0;
        nPartUnconf = 0;
        nPartStaked = 0;
        nPartImmature = 0;
        nPartWatchOnly = 0;
        nPartWatchOnlyUnconf = 0;
        nPartWatchOnlyStaked = 0;
        
        nBlind = 0;
        nBlindUnconf = 0;
        
        nAnon = 0;
        nAnonUnconf = 0;
    };
    
    CAmount nPart = 0;
    CAmount nPartUnconf = 0;
    CAmount nPartStaked = 0;
    CAmount nPartImmature = 0;
    CAmount nPartWatchOnly = 0;
    CAmount nPartWatchOnlyUnconf = 0;
    CAmount nPartWatchOnlyStaked = 0;
    
    CAmount nBlind = 0;
    CAmount nBlindUnconf = 0;
    
    CAmount nAnon = 0;
    CAmount nAnonUnconf = 0;
};

class CHDWallet : public CWallet
{
public:
    // Create wallet with dummy database handle
    CHDWallet() : CWallet()
    {
        SetHDWalletNull();
    }

    // Create wallet with passed-in database handle
    CHDWallet(std::unique_ptr<CWalletDBWrapper> dbw_in) : CWallet(std::move(dbw_in))
    {
        SetHDWalletNull();
    }
    
    void SetHDWalletNull()
    {
        nReserveBalance = 0;
        
        pEKMaster = nullptr;
        
        fUnlockForStakingOnly = false;
        nRCTOutSelectionGroup1 = 2400;
        nRCTOutSelectionGroup2 = 24000;
    };
    
    ~CHDWallet()
    {
        Finalise();
    };
    
    int Finalise();
    int FreeExtKeyMaps();
    
    /** Returns the wallets help message */
    static std::string GetWalletHelpString(bool showDebug);
    
    static bool InitLoadWallet();
    
    /* Returns true if HD is enabled, and default account set */
    bool IsHDEnabled() const override;
    
    bool DumpJson(UniValue &rv, std::string &sError);
    bool LoadJson(const UniValue &inj, std::string &sError);
    
    bool LoadAddressBook(CHDWalletDB *pwdb);
    
    bool LoadVoteTokens(CHDWalletDB *pwdb);
    bool GetVote(int nHeight, uint32_t &token);
    
    bool LoadTxRecords(CHDWalletDB *pwdb);
    
    bool EncryptWallet(const SecureString &strWalletPassphrase);
    bool Lock();
    bool Unlock(const SecureString &strWalletPassphrase);
    
    
    bool HaveAddress(const CBitcoinAddress &address);
    bool HaveKey(const CKeyID &address, CEKAKey &ak, CExtKeyAccount *&pa) const;
    bool HaveKey(const CKeyID &address) const;
    
    bool HaveExtKey(const CKeyID &address) const;
    
    bool HaveTransaction(const uint256 &txhash) const;
    
    int GetKey(const CKeyID &address, CKey &keyOut, CExtKeyAccount *&pa, CEKAKey &ak, CKeyID &idStealth) const;
    bool GetKey(const CKeyID &address, CKey &keyOut) const;
    
    bool GetPubKey(const CKeyID &address, CPubKey &pkOut) const;
    
    bool GetKeyFromPool(CPubKey &key);
    
    bool HaveStealthAddress(const CStealthAddress &sxAddr) const;
    bool GetStealthAddressScanKey(CStealthAddress &sxAddr) const;
    
    bool ImportStealthAddress(const CStealthAddress &sxAddr, const CKey &skSpend);
    
    bool AddressBookChangedNotify(const CTxDestination &address, ChangeType nMode);
    bool SetAddressBook(CHDWalletDB *pwdb, const CTxDestination &address, const std::string &strName,
        const std::string &purpose, const std::vector<uint32_t> &vPath, bool fNotifyChanged=true);
    bool SetAddressBook(const CTxDestination &address, const std::string &strName, const std::string &strPurpose);
    bool DelAddressBook(const CTxDestination &address);
    
    
    int64_t GetOldestActiveAccountTime();
    int64_t CountActiveAccountKeys();
    
    isminetype IsMine(const CTxIn& txin) const override;
    isminetype IsMine(const CScript &scriptPubKey, CKeyID &keyID,
        CEKAKey &ak, CExtKeyAccount *&pa, bool &isInvalid, SigVersion = SIGVERSION_BASE);
    
    isminetype IsMine(const CTxOutBase *txout) const ;
    bool IsMine(const CTransaction& tx) const override;
    bool IsFromMe(const CTransaction& tx) const;
    
    
    
    /**
     * Returns amount of debit if the input matches the
     * filter, otherwise returns 0
     */
    CAmount GetDebit(const CTxIn& txin, const isminefilter& filter) const override;
    CAmount GetDebit(const CTransaction& tx, const isminefilter& filter) const override;
    CAmount GetDebit(CHDWalletDB *pwdb, const CTransactionRecord &rtx, const isminefilter& filter) const;
    
    
    CAmount GetCredit(const CTxOutBase *txout, const isminefilter &filter) const;
    CAmount GetCredit(const CTransaction &tx, const isminefilter &filter) const;
    
    void GetCredit(const CTransaction &tx, CAmount &nSpendable, CAmount &nWatchOnly) const;
    
    int GetDepthInMainChain(const uint256 &blockhash, int nIndex = 0) const;
    bool InMempool(const uint256 &hash) const;
    bool IsTrusted(const uint256 &hash, const uint256 &blockhash, int nIndex = 0) const;
    
    CAmount GetBalance() const;
    CAmount GetStakeableBalance() const;        // Includes watch_only
    CAmount GetUnconfirmedBalance() const;
    CAmount GetBlindBalance();
    CAmount GetAnonBalance();
    CAmount GetStaked();
    CAmount GetLegacyBalance(const isminefilter& filter, int minDepth, const std::string* account) const override;
    
    bool GetBalances(CHDWalletBalances &bal);
    
    
    bool IsChange(const CTxOutBase *txout) const;
    
    int GetChangeAddress(CPubKey &pk);
    
    void AddOutputRecordMetaData(CTransactionRecord &rtx, std::vector<CTempRecipient> &vecSend);
    int ExpandTempRecipients(std::vector<CTempRecipient> &vecSend, CStoredExtKey *pc, std::string &sError);
    
    int CreateOutput(OUTPUT_PTR<CTxOutBase> &txbout, CTempRecipient &r, std::string &sError);
    int AddCTData(CTxOutBase *txout, CTempRecipient &r, std::string &sError);
    
    bool SetChangeDest(const CCoinControl *coinControl, CTempRecipient &r, std::string &sError);
    
    /** Update wallet after successfull transaction */
    int PostProcessTempRecipients(std::vector<CTempRecipient> &vecSend);
    
    int AddStandardInputs(CWalletTx &wtx, CTransactionRecord &rtx,
        std::vector<CTempRecipient> &vecSend,
        CExtKeyAccount *sea, CStoredExtKey *pc,
        bool sign, CAmount &nFeeRet, const CCoinControl *coinControl, std::string &sError);
    int AddStandardInputs(CWalletTx &wtx, CTransactionRecord &rtx,
        std::vector<CTempRecipient> &vecSend, bool sign, CAmount &nFeeRet, const CCoinControl *coinControl, std::string &sError);
    
    int AddBlindedInputs(CWalletTx &wtx, CTransactionRecord &rtx,
        std::vector<CTempRecipient> &vecSend,
        CExtKeyAccount *sea, CStoredExtKey *pc,
        bool sign, CAmount &nFeeRet, const CCoinControl *coinControl, std::string &sError);
    int AddBlindedInputs(CWalletTx &wtx, CTransactionRecord &rtx,
        std::vector<CTempRecipient> &vecSend, bool sign, CAmount &nFeeRet, const CCoinControl *coinControl, std::string &sError);
    
    
    int PlaceRealOutputs(std::vector<std::vector<int64_t> > &vMI, size_t &nSecretColumn, size_t nRingSize, std::set<int64_t> &setHave,
        const std::vector<std::pair<MapRecords_t::const_iterator,unsigned int> > &vCoins, std::vector<uint8_t> &vInputBlinds, std::string &sError);
    int PickHidingOutputs(std::vector<std::vector<int64_t> > &vMI, size_t nSecretColumn, size_t nRingSize, std::set<int64_t> &setHave,
        std::string &sError);
    
    int AddAnonInputs(CWalletTx &wtx, CTransactionRecord &rtx,
        std::vector<CTempRecipient> &vecSend,
        CExtKeyAccount *sea, CStoredExtKey *pc,
        bool sign, size_t nRingSize, size_t nInputsPerSig, CAmount &nFeeRet, const CCoinControl *coinControl, std::string &sError);
    int AddAnonInputs(CWalletTx &wtx, CTransactionRecord &rtx,
        std::vector<CTempRecipient> &vecSend, bool sign, size_t nRingSize, size_t nInputsPerSig, CAmount &nFeeRet, const CCoinControl *coinControl, std::string &sError);
    
    
    
    bool LoadToWallet(const CWalletTx& wtxIn);
    bool LoadToWallet(const uint256 &hash, const CTransactionRecord &rtx);
    
    /** Remove txn from mapwallet and TxSpends */
    void RemoveFromTxSpends(const uint256 &hash, const CTransactionRef pt);
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
    int ExtKeyAddAccountToMaps(CKeyID &idAccount, CExtKeyAccount *sea, bool fAddToLookAhead = true);
    int ExtKeyRemoveAccountFromMapsAndFree(CExtKeyAccount *sea);
    int ExtKeyLoadAccountPacks();
    int PrepareLookahead();

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
    int ExtKeyGetIndex(CExtKeyAccount *sea, uint32_t &index);

    int NewKeyFromAccount(CHDWalletDB *pwdb, const CKeyID &idAccount, CPubKey &pkOut, bool fInternal, bool fHardened, bool f256bit=false, const char *plabel=nullptr);
    int NewKeyFromAccount(CPubKey &pkOut, bool fInternal=false, bool fHardened=false, bool f256bit=false, const char *plabel=nullptr); // wrapper - use default account

    int NewStealthKeyFromAccount(CHDWalletDB *pwdb, const CKeyID &idAccount, std::string &sLabel, CEKAStealthKey &akStealthOut, uint32_t nPrefixBits, const char *pPrefix);
    int NewStealthKeyFromAccount(std::string &sLabel, CEKAStealthKey &akStealthOut, uint32_t nPrefixBits, const char *pPrefix); // wrapper - use default account

    int NewExtKeyFromAccount(CHDWalletDB *pwdb, const CKeyID &idAccount, std::string &sLabel, CStoredExtKey *sekOut, const char *plabel=nullptr, uint32_t *childNo=nullptr);
    int NewExtKeyFromAccount(std::string &sLabel, CStoredExtKey *sekOut, const char *plabel=nullptr, uint32_t *childNo=nullptr); // wrapper - use default account

    int ExtKeyGetDestination(const CExtKeyPair &ek, CPubKey &pkDest, uint32_t &nKey);
    int ExtKeyUpdateLooseKey(const CExtKeyPair &ek, uint32_t nKey, bool fAddToAddressBook);
    
    int ScanChainFromTime(int64_t nTimeStartScan);
    int ScanChainFromHeight(int nHeight);
    
    /**
     * Insert additional inputs into the transaction by
     * calling CreateTransaction();
     */
    bool FundTransaction(CMutableTransaction& tx, CAmount& nFeeRet, int& nChangePosInOut, std::string& strFailReason, bool lockUnspents, const std::set<int>& setSubtractFeeFromOutputs, CCoinControl);
    
    bool CreateTransaction(const std::vector<CRecipient>& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet, int& nChangePosInOut,
                           std::string& strFailReason, const CCoinControl& coin_control, bool sign = true) override;
    bool CommitTransaction(CWalletTx &wtxNew, CReserveKey &reservekey, CConnman *connman, CValidationState &state);
    bool CommitTransaction(CWalletTx &wtxNew, CTransactionRecord &rtx,
        CReserveKey &reservekey, CConnman *connman, CValidationState &state);
    
    int LoadStealthAddresses();
    bool IndexStealthKey(CHDWalletDB *pwdb, uint160 &hash, const CStealthAddressIndexed &sxi, uint32_t &id);
    bool GetStealthKeyIndex(const CStealthAddressIndexed &sxi, uint32_t &id);
    bool UpdateStealthAddressIndex(const CKeyID &idK, const CStealthAddressIndexed &sxi, uint32_t &id); // Get stealth index or create new index if none found
    bool GetStealthByIndex(uint32_t sxId, CStealthAddress &sx) const;
    bool GetStealthLinked(const CKeyID &idK, CStealthAddress &sx);
    bool ProcessLockedStealthOutputs();
    bool ProcessLockedAnonOutputs();
    bool ProcessStealthOutput(const CTxDestination &address,
        std::vector<uint8_t> &vchEphemPK, uint32_t prefix, bool fHavePrefix, CKey &sShared, bool fNeedShared=false);
    
    int CheckForStealthAndNarration(const CTxOutBase *pb, const CTxOutData *pdata, std::string &sNarr);
    bool FindStealthTransactions(const CTransaction &tx, mapValue_t &mapNarr);
    
    bool ScanForOwnedOutputs(const CTransaction &tx, size_t &nCT, size_t &nRingCT, mapValue_t &mapNarr);
    bool AddToWalletIfInvolvingMe(const CTransactionRef& ptx, const CBlockIndex* pIndex, int posInBlock, bool fUpdate);
    
    CWalletTx *GetTempWalletTx(const uint256& hash);
    
    const CWalletTx *GetWalletTx(const uint256& hash) const;
    CWalletTx *GetWalletTx(const uint256& hash);
    
    int InsertTempTxn(const uint256 &txid, const CTransactionRecord *rtx) const;
    
    int OwnStandardOut(const CTxOutStandard *pout, const CTxOutData *pdata, COutputRecord &rout, bool &fUpdated);
    int OwnBlindOut(CHDWalletDB *pwdb, const uint256 &txhash, const CTxOutCT *pout, const CStoredExtKey *pc, uint32_t &nLastChild,
        COutputRecord &rout, CStoredTransaction &stx, bool &fUpdated);
    int OwnAnonOut(CHDWalletDB *pwdb, const uint256 &txhash, const CTxOutRingCT *pout, const CStoredExtKey *pc, uint32_t &nLastChild,
        COutputRecord &rout, CStoredTransaction &stx, bool &fUpdated);
    
    bool AddTxinToSpends(const CTxIn &txin, const uint256 &txhash);
    
    bool AddToRecord(CTransactionRecord &rtxIn, const CTransaction &tx,
        const CBlockIndex *pIndex, int posInBlock, bool fFlushOnClose=true);
    
    int GetRequestCount(const uint256 &hash, const CTransactionRecord &rtx);
    
    std::vector<uint256> ResendRecordTransactionsBefore(int64_t nTime, CConnman *connman);
    void ResendWalletTransactions(int64_t nBestBlockTime, CConnman *connman) override;
    
    
    /**
     * populate vCoins with vector of available COutputs.
     */
    void AvailableCoins(std::vector<COutput>& vCoins, bool fOnlySafe=true, const CCoinControl *coinControl = nullptr, const CAmount& nMinimumAmount = 1, const CAmount& nMaximumAmount = MAX_MONEY, const CAmount& nMinimumSumAmount = MAX_MONEY, const uint64_t& nMaximumCount = 0, const int& nMinDepth = 0, const int& nMaxDepth = 0x7FFFFFFF, bool fIncludeImmature=false) const override;
    bool SelectCoins(const std::vector<COutput>& vAvailableCoins, const CAmount& nTargetValue, std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet, const CCoinControl *coinControl = nullptr) const;

    void AvailableBlindedCoins(std::vector<COutputR>& vCoins, bool fOnlySafe=true, const CCoinControl *coinControl = nullptr, const CAmount& nMinimumAmount = 1, const CAmount& nMaximumAmount = MAX_MONEY, const CAmount& nMinimumSumAmount = MAX_MONEY, const uint64_t& nMaximumCount = 0, const int& nMinDepth = 0, const int& nMaxDepth = 0x7FFFFFFF, bool fIncludeImmature=false) const;
    bool SelectBlindedCoins(const std::vector<COutputR>& vAvailableCoins, const CAmount& nTargetValue, std::vector<std::pair<MapRecords_t::const_iterator,unsigned int> > &setCoinsRet, CAmount &nValueRet, const CCoinControl *coinControl = nullptr) const;

    void AvailableAnonCoins(std::vector<COutputR> &vCoins, bool fOnlySafe=true, const CCoinControl *coinControl = nullptr, const CAmount& nMinimumAmount = 1, const CAmount& nMaximumAmount = MAX_MONEY, const CAmount& nMinimumSumAmount = MAX_MONEY, const uint64_t& nMaximumCount = 0, const int& nMinDepth = 0, const int& nMaxDepth = 0x7FFFFFFF, bool fIncludeImmature=false) const;
    //bool SelectAnonCoins(const std::vector<COutputR> &vAvailableCoins, const CAmount &nTargetValue, std::vector<std::pair<MapRecords_t::const_iterator,unsigned int> > &setCoinsRet, CAmount &nValueRet, const CCoinControl *coinControl = NULL) const;
    
    bool SelectCoinsMinConf(const CAmount& nTargetValue, int nConfMine, int nConfTheirs, uint64_t nMaxAncestors, std::vector<COutputR> vCoins, std::vector<std::pair<MapRecords_t::const_iterator,unsigned int> > &setCoinsRet, CAmount &nValueRet) const;
    
    bool IsSpent(const uint256& hash, unsigned int n) const;
    
    std::set<uint256> GetConflicts(const uint256 &txid) const;
    
    /* Mark a transaction (and it in-wallet descendants) as abandoned so its inputs may be respent. */
    bool AbandonTransaction(const uint256 &hashTx) override;
    
    void MarkConflicted(const uint256 &hashBlock, const uint256 &hashTx);
    void SyncMetaData(std::pair<TxSpends::iterator, TxSpends::iterator>);
    
    bool GetSetting(const std::string &setting, UniValue &json);
    bool SetSetting(const std::string &setting, const UniValue &json);
    bool EraseSetting(const std::string &setting);
    
    bool SetReserveBalance(CAmount nNewReserveBalance);
    uint64_t GetStakeWeight() const;
    void AvailableCoinsForStaking(std::vector<COutput> &vCoins, int64_t nTime, int nHeight) const;
    bool SelectCoinsForStaking(int64_t nTargetValue, int64_t nTime, int nHeight, std::set<std::pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet) const;
    bool CreateCoinStake(unsigned int nBits, int64_t nTime, int nBlockHeight, int64_t nFees, CMutableTransaction &txNew, CKey &key);
    bool SignBlock(CBlockTemplate *pblocktemplate, int nHeight, int64_t nSearchTime);
    
    int64_t nLastCoinStakeSearchTime = 0;
    uint32_t nStealth, nFoundStealth; // for reporting, zero before use
    int64_t nReserveBalance;
    size_t nStakeThread = 9999999; // unset
    mutable int deepestTxnDepth = 0; // for stake mining
    int nStakeLimitHeight = 0; // for regtest, don't stake above nStakeLimitHeight
    
    enum eStakingState {
        NOT_STAKING = 0,
        IS_STAKING = 1,
        NOT_STAKING_BALANCE = -1,
        NOT_STAKING_DEPTH = -2,
        NOT_STAKING_LOCKED = -3,
        NOT_STAKING_LIMITED = -4,
    } nIsStaking = NOT_STAKING;
    
    std::set<CStealthAddress> stealthAddresses;
    
    CStoredExtKey *pEKMaster;
    CKeyID idDefaultAccount;
    ExtKeyAccountMap mapExtAccounts;
    ExtKeyMap mapExtKeys;
    
    mutable MapWallet_t mapTempWallet;
    
    MapRecords_t mapRecords;
    RtxOrdered_t rtxOrdered;
    
    std::vector<CVoteToken> vVoteTokens;
    
    int nUserDevFundCedePercent;
    bool fUnlockForStakingOnly; // temporary, non-optimal solution.
    
    int64_t nRCTOutSelectionGroup1;
    int64_t nRCTOutSelectionGroup2;
    
};

int ToStealthRecipient(CStealthAddress &sx, CAmount nValue, bool fSubtractFeeFromAmount,
    std::vector<CRecipient> &vecSend, std::string &sNarr, std::string &strError);

class LoopExtKeyCallback
{
public:
    CHDWallet *pwallet = nullptr;
    
    // NOTE: the key and account instances passed to Process are temporary
    virtual int ProcessKey(CKeyID &id, CStoredExtKey &sek) {return 1;};
    virtual int ProcessAccount(CKeyID &id, CExtKeyAccount &sek) {return 1;};
};

int LoopExtKeysInDB(CHDWallet *pwallet, bool fInactive, bool fInAccount, LoopExtKeyCallback &callback);
int LoopExtAccountsInDB(CHDWallet *pwallet, bool fInactive, LoopExtKeyCallback &callback);

bool IsHDWallet(CWallet *win);
CHDWallet *GetHDWallet(CWallet *win);

#endif // PARTICL_WALLET_HDWALLET_H
