// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_WALLET_H
#define BITCOIN_WALLET_H

#include "core.h"
#include "crypter.h"
#include "key.h"
#include "keystore.h"
#include "main.h"
#include "ui_interface.h"
#include "util.h"
#include "walletdb.h"

#include "bitcoin_wallet.h"

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

// Settings
extern int64_t credits_nTransactionFee;
extern bool credits_bSpendZeroConfChange;

// -paytxfee default
static const int64_t CREDITS_DEFAULT_TRANSACTION_FEE = 0;
// -paytxfee will warn if called with a higher fee than this amount (in satoshis) per KB
static const int credits_nHighTransactionFeeWarning = 0.01 * COIN;

class CAccountingEntry;
class CCoinControl;
class Credits_COutput;
class Credits_CReserveKey;
class CScript;
class Credits_CWalletTx;
class Bitcoin_COutput;

/** (client) version numbers for particular wallet features */
enum Credits_WalletFeature
{
    CREDITS_FEATURE_BASE = 10500, // the earliest version new wallets supports (only useful for getinfo's clientversion output)

    CREDITS_FEATURE_WALLETCRYPT = 40000, // wallet encryption
    CREDITS_FEATURE_COMPRPUBKEY = 60000, // compressed public keys

    CREDITS_FEATURE_LATEST = 60000
};


/** A CWallet is an extension of a keystore, which also maintains a set of transactions and balances,
 * and provides the ability to create new transactions.
 */
class Credits_CWallet : public CCryptoKeyStore, public Credits_CWalletInterface
{
private:
	bool ImportKeyFromBitcoinWallet (CTxDestination &address, Bitcoin_CWallet * bitcoinWallet);
    bool SelectCoins(int64_t nTargetValue, std::set<std::pair<const Credits_CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet, map<uint256, set<int> >& mapFilterTxInPoints, const CCoinControl *coinControl = NULL) const;

    Credits_CWalletDB *pwalletdbEncryption;

    // the current wallet version: clients below this version are not able to load the wallet
    int nWalletVersion;

    // the maximum wallet format version: memory-only variable that specifies to what version this wallet may be upgraded
    int nWalletMaxVersion;

    int64_t nNextResend;
    int64_t nLastResend;

    // Used to keep track of spent outpoints, and
    // detect and report conflicts (double-spends or
    // mutated transactions where the mutant gets mined).
    typedef std::multimap<COutPoint, uint256> TxSpends;
    TxSpends mapTxSpends;
    void AddToSpends(const COutPoint& outpoint, const uint256& wtxid);
    void AddToSpends(const uint256& wtxid);

    void SyncMetaData(std::pair<TxSpends::iterator, TxSpends::iterator>);

    void NotifyTxPrevInsBitcoin(Bitcoin_CWallet *bitcoin_wallet, Credits_CWalletTx &wtx);
    void NotifyTxPrevInsCredits(Credits_CWallet *credits_wallet, Credits_CWalletTx &wtx);

public:
    Credits_CDBEnv *pbitDb;
    /// Main wallet lock.
    /// This lock protects all the fields added by CWallet
    ///   except for:
    ///      fFileBacked (immutable after instantiation)
    ///      strWalletFile (immutable after instantiation)
    mutable CCriticalSection cs_wallet;

    bool fFileBacked;
    std::string strWalletFile;

    std::set<int64_t> setKeyPool;
    std::map<CKeyID, Credits_CKeyMetadata> mapKeyMetadata;

    typedef std::map<unsigned int, CMasterKey> MasterKeyMap;
    MasterKeyMap mapMasterKeys;
    unsigned int nMasterKeyMaxID;

    Credits_CWallet(Credits_CDBEnv *pbitDbIn)
    {
        SetNull(pbitDbIn);
    }
    Credits_CWallet(std::string strWalletFileIn, Credits_CDBEnv *pbitDbIn)
    {
        SetNull(pbitDbIn);

        strWalletFile = strWalletFileIn;
        fFileBacked = true;
    }
    void SetNull(Credits_CDBEnv *pbitDbIn)
    {
        nWalletVersion = CREDITS_FEATURE_BASE;
        nWalletMaxVersion = CREDITS_FEATURE_BASE;
        fFileBacked = false;
        nMasterKeyMaxID = 0;
        pwalletdbEncryption = NULL;
        nOrderPosNext = 0;
        nNextResend = 0;
        nLastResend = 0;
        nTimeFirstKey = 0;
    	pbitDb = pbitDbIn;
    }

    std::map<uint256, Credits_CWalletTx> mapWallet;

    int64_t nOrderPosNext;
    std::map<uint256, int> mapRequestCount;

    std::map<CTxDestination, CAddressBookData> mapAddressBook;

    CPubKey vchDefaultKey;

    std::set<COutPoint> setLockedCoins;

    int64_t nTimeFirstKey;

    const Credits_CWalletTx* GetWalletTx(const uint256& hash) const;
    void GetAllWalletTxs(vector<const Credits_CWalletTx*> &vWalletTxs) const;

    // check whether we are allowed to upgrade (or already support) to the named feature
    bool CanSupportFeature(enum Credits_WalletFeature wf) { AssertLockHeld(cs_wallet); return nWalletMaxVersion >= wf; }

    void AvailableCoins(std::vector<Credits_COutput>& vCoins, map<uint256, set<int> >& mapFilterTxInPoints, bool fOnlyConfirmed=true, const CCoinControl *coinControl = NULL) const;
    void ClaimTxInPoints(map<uint256, set<int> >& mapClaimTxPoints) const;
    void PreparedDepositTxInPoints(map<uint256, set<int> >& mapDepositTxPoints) const;
    bool SelectCoinsMinConf(int64_t nTargetValue, int nConfMine, int nConfTheirs, std::vector<Credits_COutput> vCoins, std::set<std::pair<const Credits_CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet) const;

    bool IsSpent(const uint256& hash, unsigned int n) const;

    bool IsLockedCoin(uint256 hash, unsigned int n) const;
    void LockCoin(COutPoint& output);
    void UnlockCoin(COutPoint& output);
    void UnlockAllCoins();
    void ListLockedCoins(std::vector<COutPoint>& vOutpts);

    // keystore implementation
    // Generate a new key
    CPubKey GenerateNewKey();
    // Adds a key to the store, and saves it to disk.
    bool AddKeyPubKey(const CKey& key, const CPubKey &pubkey);
    // Adds a key to the store, without saving it to disk (used by LoadWallet)
    bool LoadKey(const CKey& key, const CPubKey &pubkey) { return CCryptoKeyStore::AddKeyPubKey(key, pubkey); }
    // Load metadata (used by LoadWallet)
    bool LoadKeyMetadata(const CPubKey &pubkey, const Credits_CKeyMetadata &metadata);

    bool LoadMinVersion(int nVersion) { AssertLockHeld(cs_wallet); nWalletVersion = nVersion; nWalletMaxVersion = std::max(nWalletMaxVersion, nVersion); return true; }

    // Adds an encrypted key to the store, and saves it to disk.
    bool AddCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);
    // Adds an encrypted key to the store, without saving it to disk (used by LoadWallet)
    bool LoadCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);
    bool AddCScript(const CScript& redeemScript);
    bool LoadCScript(const CScript& redeemScript) { return CCryptoKeyStore::AddCScript(redeemScript); }

    /// Adds a destination data tuple to the store, and saves it to disk
    bool AddDestData(const CTxDestination &dest, const std::string &key, const std::string &value);
    /// Erases a destination data tuple in the store and on disk
    bool EraseDestData(const CTxDestination &dest, const std::string &key);
    /// Adds a destination data tuple to the store, without saving it to disk
    bool LoadDestData(const CTxDestination &dest, const std::string &key, const std::string &value);
    /// Look up a destination data tuple in the store, return true if found false otherwise
    bool GetDestData(const CTxDestination &dest, const std::string &key, std::string *value) const;

    bool Unlock(const SecureString& strWalletPassphrase);
    bool ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase);
    bool EncryptWallet(const SecureString& strWalletPassphrase);

    void GetKeyBirthTimes(std::map<CKeyID, int64_t> &mapKeyBirth) const;

    /** Increment the next transaction order id
        @return next transaction order id
     */
    int64_t IncOrderPosNext(Credits_CWalletDB *pwalletdb = NULL);

    typedef std::pair<Credits_CWalletTx*, CAccountingEntry*> TxPair;
    typedef std::multimap<int64_t, TxPair > TxItems;

    /** Get the wallet's activity log
        @return multimap of ordered transactions and accounting entries
        @warning Returned pointers are *only* valid within the scope of passed acentries
     */
    TxItems OrderedTxItems(std::list<CAccountingEntry>& acentries, std::string strAccount = "");

    void MarkDirty();
    bool AddToWallet(const Credits_CWalletTx& wtxIn, bool fFromLoadWallet=false);
    void SyncTransaction(const Bitcoin_CWallet *bitcoin_wallet, const uint256 &hash, const Credits_CTransaction& tx, const Credits_CBlock* pblock);
    bool AddToWalletIfInvolvingMe(const Bitcoin_CWallet *bitcoin_wallet, const uint256 &hash, const Credits_CTransaction& tx, const Credits_CBlock* pblock, bool fUpdate);
    void EraseFromWallet(Credits_CWallet *credits_wallet, const uint256 &hash);
    int ScanForWalletTransactions(const Bitcoin_CWallet *bitcoin_wallet, Bitcoin_CClaimCoinsViewCache *claim_view, Credits_CBlockIndex* pindexStart, bool fUpdate = false);
    void ReacceptWalletTransactions();
    void ResendWalletTransactions();
    int64_t GetBalance(map<uint256, set<int> >& mapFilterTxInPoints) const;
    int64_t GetUnconfirmedBalance(map<uint256, set<int> >& mapFilterTxInPoints) const;
    int64_t GetImmatureBalance(map<uint256, set<int> >& mapFilterTxInPoints) const;
    int64_t GetPreparedDepositBalance() const;
    int64_t GetInDepositBalance() const;
    bool CreateTransaction(Credits_CWallet *deposit_wallet, const std::vector<std::pair<CScript, int64_t> >& vecSend,
                           Credits_CWalletTx& wtxNew, Credits_CReserveKey& reservekey, int64_t& nFeeRet, std::string& strFailReason, const CCoinControl *coinControl = NULL);
    bool CreateTransaction(Credits_CWallet *deposit_wallet, CScript scriptPubKey, int64_t nValue,
                           Credits_CWalletTx& wtxNew, Credits_CReserveKey& reservekey, int64_t& nFeeRet, std::string& strFailReason, const CCoinControl *coinControl = NULL);
    bool CreateDepositTransaction(Credits_CWallet *deposit_wallet, const std::pair<CScript, int64_t>& vecSend,
                           Credits_CWalletTx& wtxNew, Credits_CReserveKey& reservekey, const int64_t & nTotalIn, std::string& strFailReason, const Credits_COutput& coin, Credits_CCoinsViewCache &credits_view, Bitcoin_CClaimCoinsViewCache &claim_view);
    bool CreateClaimTransaction(Bitcoin_CWallet *bitcoin_wallet, Bitcoin_CClaimCoinsViewCache *claim_view, const std::vector<std::pair<CScript, int64_t> >& vecSend,
                           Credits_CWalletTx& wtxNew, Credits_CReserveKey& reservekey, int64_t& nFeeRet, std::string& strFailReason, const CCoinControl *coinControl = NULL);
    bool CommitTransaction(Bitcoin_CWallet *bitcoin_wallet, Credits_CWalletTx& wtxNew, Credits_CReserveKey& reservekey, std::vector<Credits_CReserveKey *> &keyRecipients);
    bool CommitDepositTransaction(Credits_CWallet *credits_wallet, Credits_CWalletTx& wtxNew, Credits_CReserveKey& reservekey, std::vector<Credits_CReserveKey *> &keyRecipients);
    std::string SendMoney(Bitcoin_CWallet *bitcoin_wallet, Credits_CWallet *deposit_wallet, CScript scriptPubKey, int64_t nValue, Credits_CWalletTx& wtxNew);
    std::string SendMoneyToDestination(Bitcoin_CWallet *bitcoin_wallet, Credits_CWallet *deposit_wallet, const CTxDestination &address, int64_t nValue, Credits_CWalletTx& wtxNew);

    bool NewKeyPool();
    bool TopUpKeyPool(unsigned int kpSize = 0);
    void ReserveKeyFromKeyPool(int64_t& nIndex, CKeyPool& keypool);
    void KeepKey(int64_t nIndex);
    void ReturnKey(int64_t nIndex);
    bool GetKeyFromPool(CPubKey &key);
    int64_t GetOldestKeyPoolTime();
    void GetAllReserveKeys(std::set<CKeyID>& setAddress) const;

    std::set< std::set<CTxDestination> > GetAddressGroupings();
    std::map<CTxDestination, int64_t> GetAddressBalances();

    std::set<CTxDestination> GetAccountAddresses(std::string strAccount) const;

    bool IsMine(const Credits_CTxIn& txin) const;
    int64_t GetDebit(const Credits_CTxIn& txin) const;
    bool IsMine(const CTxOut& txout) const
    {
        return ::IsMine(*this, txout.scriptPubKey);
    }
    int64_t GetCredit(const CTxOut& txout) const
    {
    	assert_with_stacktrace(Credits_MoneyRange(txout.nValue), "Credits: CWallet::GetCredit() : value out of range");
        return (IsMine(txout) ? txout.nValue : 0);
    }
    bool IsChange(const CTxOut& txout) const;
    int64_t GetChange(const CTxOut& txout) const
    {
    	assert_with_stacktrace(Credits_MoneyRange(txout.nValue), "Credits: CWallet::GetChange() : value out of range");
        return (IsChange(txout) ? txout.nValue : 0);
    }
    bool IsMine(const Credits_CTransaction& tx) const
    {
        BOOST_FOREACH(const CTxOut& txout, tx.vout)
            if (IsMine(txout))
                return true;
        return false;
    }
    bool IsFromMe(const Credits_CTransaction& tx) const
    {
        return (GetDebit(tx) > 0);
    }
    int64_t GetDebit(const Credits_CTransaction& tx) const
    {
        int64_t nDebit = 0;
        BOOST_FOREACH(const Credits_CTxIn& txin, tx.vin)
        {
            nDebit += GetDebit(txin);
            assert_with_stacktrace(Credits_MoneyRange(nDebit), "Credits: CWallet::GetDebit() : value out of range");
        }
        return nDebit;
    }
    int64_t GetCredit(const Credits_CTransaction& tx, map<uint256, set<int> >& mapFilterTxInPoints) const
    {
    	const uint256 &hashTx = tx.GetHash();
        int64_t nCredit = 0;
		for(unsigned int i = 0; i < tx.vout.size(); i++) {
			if(!IsInFilterPoints(hashTx, i, mapFilterTxInPoints)) {
				const CTxOut& txout = tx.vout[i];
				nCredit += GetCredit(txout);
				assert_with_stacktrace(Credits_MoneyRange(nCredit), "Credits: CWallet::GetCredit() : value out of range");
			}
        }
        return nCredit;
    }
    int64_t GetChange(const Credits_CTransaction& tx) const
    {
        int64_t nChange = 0;
        BOOST_FOREACH(const CTxOut& txout, tx.vout)
        {
            nChange += GetChange(txout);
            assert_with_stacktrace(Credits_MoneyRange(nChange), "Credits: CWallet::GetChange() : value out of range");
        }
        return nChange;
    }
    void SetBestChain(const CBlockLocator& loc);

    Credits_DBErrors LoadWallet(bool& fFirstRunRet, uint64_t &nAccountingEntryNumber);
    Credits_DBErrors ZapWalletTx();

    bool SetAddressBook(const CTxDestination& address, const std::string& strName, const std::string& purpose);

    bool DelAddressBook(const CTxDestination& address);

    void UpdatedTransaction(const uint256 &hashTx);

    void Inventory(const uint256 &hash)
    {
        {
            LOCK(cs_wallet);
            std::map<uint256, int>::iterator mi = mapRequestCount.find(hash);
            if (mi != mapRequestCount.end())
                (*mi).second++;
        }
    }

    unsigned int GetKeyPoolSize()
    {
        AssertLockHeld(cs_wallet); // setKeyPool
        return setKeyPool.size();
    }

    bool SetDefaultKey(const CPubKey &vchPubKey);

    // signify that a particular wallet feature is now used. this may change nWalletVersion and nWalletMaxVersion if those are lower
    bool SetMinVersion(enum Credits_WalletFeature, Credits_CWalletDB* pwalletdbIn = NULL, bool fExplicit = false);

    // change which version we're allowed to upgrade to (note that this does not immediately imply upgrading to that format)
    bool SetMaxVersion(int nVersion);

    // get the current wallet format (the oldest client version guaranteed to understand this wallet)
    int GetVersion() { LOCK(cs_wallet); return nWalletVersion; }

    // Get wallet transactions that conflict with given transaction (spend same outputs)
    std::set<uint256> GetConflicts(const uint256& txid) const;

    /** Address book entry changed.
     * @note called with lock cs_wallet held.
     */
    boost::signals2::signal<void (Credits_CWallet *wallet, const CTxDestination
            &address, const std::string &label, bool isMine,
            const std::string &purpose,
            ChangeType status)> NotifyAddressBookChanged;

    /** Wallet transaction added, removed or updated.
     * @note called with lock cs_wallet held.
     */
    boost::signals2::signal<void (Credits_CWallet *wallet, const uint256 &hashTx,
            ChangeType status)> NotifyTransactionChanged;

    /** Show progress e.g. for rescan */
    boost::signals2::signal<void (const std::string &title, int nProgress)> ShowProgress;
};

/** A key allocated from the key pool. */
class Credits_CReserveKey
{
protected:
    Credits_CWallet* pwallet;
    int64_t nIndex;
    CPubKey vchPubKey;
public:
    Credits_CReserveKey(Credits_CWallet* pwalletIn)
    {
        nIndex = -1;
        pwallet = pwalletIn;
    }

    ~Credits_CReserveKey()
    {
        ReturnKey();
    }

    void ReturnKey();
    bool GetReservedKey(CPubKey &pubkey);
    void KeepKey();
};


/** A transaction with a bunch of additional info that only the owner cares about.
 * It includes any unrecorded transactions needed to link it back to the block chain.
 */
class Credits_CWalletTx : public Credits_CMerkleTx
{
private:
    const Credits_CWallet* pwallet;

public:
    mapValue_t mapValue;
    std::vector<std::pair<std::string, std::string> > vOrderForm;
    unsigned int fTimeReceivedIsTxTime;
    unsigned int nTimeReceived;  // time received by this node
    unsigned int nTimeSmart;
    char fFromMe;
    std::string strFromAccount;
    int64_t nOrderPos;  // position in ordered transaction list

    // memory only
    mutable bool fDebitCached;
    mutable bool fCreditCached;
    mutable bool fImmatureCreditCached;
    mutable bool fAvailableCreditCached;
    mutable bool fPreparedDepositCreditCached;
    mutable bool fInDepositCreditCached;
    mutable bool fChangeCached;
    mutable int64_t nDebitCached;
    mutable int64_t nCreditCached;
    mutable int64_t nImmatureCreditCached;
    mutable int64_t nAvailableCreditCached;
    mutable int64_t nPreparedDepositCreditCached;
    mutable int64_t nInDepositCreditCached;
    mutable int64_t nChangeCached;

    Credits_CWalletTx()
    {
        Init(NULL);
    }

    Credits_CWalletTx(const Credits_CWallet* pwalletIn)
    {
        Init(pwalletIn);
    }

    Credits_CWalletTx(const Credits_CWallet* pwalletIn, const Credits_CMerkleTx& txIn) : Credits_CMerkleTx(txIn)
    {
        Init(pwalletIn);
    }

    Credits_CWalletTx(const Credits_CWallet* pwalletIn, const Credits_CTransaction& txIn) : Credits_CMerkleTx(txIn)
    {
        Init(pwalletIn);
    }

    void Init(const Credits_CWallet* pwalletIn)
    {
        pwallet = pwalletIn;
        mapValue.clear();
        vOrderForm.clear();
        fTimeReceivedIsTxTime = false;
        nTimeReceived = 0;
        nTimeSmart = 0;
        fFromMe = false;
        strFromAccount.clear();
        fDebitCached = false;
        fCreditCached = false;
        fImmatureCreditCached = false;
        fAvailableCreditCached = false;
        fPreparedDepositCreditCached = false;
        fInDepositCreditCached = false;
        fChangeCached = false;
        nDebitCached = 0;
        nCreditCached = 0;
        nImmatureCreditCached = 0;
        nAvailableCreditCached = 0;
        nPreparedDepositCreditCached = 0;
        nInDepositCreditCached = 0;
        nChangeCached = 0;
        nOrderPos = -1;
    }

    IMPLEMENT_SERIALIZE
    (
        Credits_CWalletTx* pthis = const_cast<Credits_CWalletTx*>(this);
        if (fRead)
            pthis->Init(NULL);
        char fSpent = false;

        if (!fRead)
        {
            pthis->mapValue["fromaccount"] = pthis->strFromAccount;

            WriteOrderPos(pthis->nOrderPos, pthis->mapValue);

            if (nTimeSmart)
                pthis->mapValue["timesmart"] = strprintf("%u", nTimeSmart);
        }

        nSerSize += SerReadWrite(s, *(Credits_CMerkleTx*)this, nType, nVersion,ser_action);
        std::vector<Credits_CMerkleTx> vUnused; // Used to be vtxPrev
        READWRITE(vUnused);
        READWRITE(mapValue);
        READWRITE(vOrderForm);
        READWRITE(fTimeReceivedIsTxTime);
        READWRITE(nTimeReceived);
        READWRITE(fFromMe);
        READWRITE(fSpent);

        if (fRead)
        {
            pthis->strFromAccount = pthis->mapValue["fromaccount"];

            ReadOrderPos(pthis->nOrderPos, pthis->mapValue);

            pthis->nTimeSmart = mapValue.count("timesmart") ? (unsigned int)atoi64(pthis->mapValue["timesmart"]) : 0;
        }

        pthis->mapValue.erase("fromaccount");
        pthis->mapValue.erase("version");
        pthis->mapValue.erase("spent");
        pthis->mapValue.erase("n");
        pthis->mapValue.erase("timesmart");
    )

    // make sure balances are recalculated
    void MarkDirty()
    {
        fCreditCached = false;
        fAvailableCreditCached = false;
        fPreparedDepositCreditCached = false;
        fInDepositCreditCached = false;
        fDebitCached = false;
        fChangeCached = false;
    }

    void BindWallet(Credits_CWallet *pwalletIn)
    {
        pwallet = pwalletIn;
        MarkDirty();
    }

    int64_t GetDebit(const Credits_CWallet *keyholder_wallet = NULL) const
    {
        if (vin.empty())
            return 0;
        if (fDebitCached)
            return nDebitCached;
        if(keyholder_wallet) {
        	nDebitCached = keyholder_wallet->GetDebit(*this);
        } else {
        	nDebitCached = pwallet->GetDebit(*this);
        }
        fDebitCached = true;
        return nDebitCached;
    }

    int64_t GetCredit(map<uint256, set<int> >& mapFilterTxInPoints, const Credits_CWallet *keyholder_wallet = NULL, bool fUseCache=true) const
    {
//        // Must wait until coinbase is safely deep enough in the chain before valuing it
//        if (IsCoinBase() && GetBlocksToMaturity() > 0)
//            return 0;
//        //...and deposits are also blocked for a certain period
//		if (IsDeposit() && vout.size() == 1 && GetFirstDepositOutBlocksToMaturity() > 0) {
//			return 0;
//		}
//		if (IsDeposit() && vout.size() == 2 && GetSecondDepositOutBlocksToMaturity() > 0) {
//			return 0;
//		}

        // GetBalance can assume transactions in mapWallet won't change
        if (fUseCache && fCreditCached)
            return nCreditCached;
        if(keyholder_wallet) {
        	nCreditCached = keyholder_wallet->GetCredit(*this, mapFilterTxInPoints);
        } else {
        	nCreditCached = pwallet->GetCredit(*this, mapFilterTxInPoints);
        }
        fCreditCached = true;
        return nCreditCached;
    }

	void AddTxOutValue(int64_t &nCredit, const uint256 &hashTx, const unsigned int &n) const {
		const CTxOut &txout = vout[n];
		if (!pwallet->IsSpent(hashTx, n)) {
			nCredit += pwallet->GetCredit(txout);
			assert_with_stacktrace(Credits_MoneyRange(nCredit), "Credits: CWalletTx::AddTxOutValue() : value out of range");
		}
	}

    int64_t GetImmatureCredit(map<uint256, set<int> >& mapFilterTxInPoints, bool fUseCache=true) const
    {
        if ( ((IsCoinBase() && GetBlocksToMaturity() > 0) || (IsDeposit() && vout.size() == 2 && GetSecondDepositOutBlocksToMaturity() > 0)) && IsInMainChain())
        {
            if (fUseCache && fImmatureCreditCached)
                return nImmatureCreditCached;

            //This loop is neccessary since coinbases CAN be spent by deposit in same block
            //This has been copied from pwallet->GetCredit()
            int64_t nCredit = 0;
			const uint256 &hashTx = GetHash();
			unsigned int start = 0;
            if(IsDeposit()) {
				start = 1;
            }

			for (unsigned int i = start; i < vout.size(); i++) {
				if(!IsInFilterPoints(hashTx, i, mapFilterTxInPoints)) {
					AddTxOutValue(nCredit, hashTx, i);
				}
			}

            nImmatureCreditCached = nCredit;

            fImmatureCreditCached = true;
            return nImmatureCreditCached;
        }

        return 0;
    }

    int64_t GetAvailableCredit(map<uint256, set<int> >& mapFilterTxInPoints, bool fUseCache=true) const
    {
        if (pwallet == 0)
            return 0;

        // Must wait until coinbase is safely deep enough in the chain before valuing it
        if (IsCoinBase() && GetBlocksToMaturity() > 0)
            return 0;
        //...and deposits are also blocked for a certain period
		if (IsDeposit() && vout.size() == 1 && GetFirstDepositOutBlocksToMaturity() > 0) {
			return 0;
		}
		if (IsDeposit() && vout.size() == 2 && GetSecondDepositOutBlocksToMaturity() > 0) {
			return 0;
		}

        if (fUseCache && fAvailableCreditCached)
            return nAvailableCreditCached;

        int64_t nCredit = 0;
        const uint256 &hashTx = GetHash();
		unsigned int start = 0;
        if(IsDeposit() && GetFirstDepositOutBlocksToMaturity() > 0) {
			start = 1;
        }
        for (unsigned int i = start; i < vout.size(); i++) {
    		if(!IsInFilterPoints(hashTx, i, mapFilterTxInPoints)) {
    			AddTxOutValue(nCredit, hashTx, i);
            }
        }

        nAvailableCreditCached = nCredit;
        fAvailableCreditCached = true;
        return nCredit;
    }

    int64_t GetPreparedDepositCredit(bool fUseCache=true) const
    {
        if (pwallet == 0)
            return 0;

		int64_t nCredit = 0;

		if (IsDeposit()) {
			if (fUseCache && fPreparedDepositCreditCached)
				return nPreparedDepositCreditCached;

			uint256 hashTx = GetHash();
			for (unsigned int i = 0; i < vout.size(); i++) {
				const CTxOut &txout = vout[i];
				if (!pwallet->IsSpent(hashTx, i)) {
					nCredit += txout.nValue;
					assert_with_stacktrace(Credits_MoneyRange(nCredit), "Credits: CWalletTx::GetImmatureCredit() : value out of range");
				}
			}

			nPreparedDepositCreditCached = nCredit;
			fPreparedDepositCreditCached = true;
        }

        return nCredit;
    }

    int64_t GetInDepositCredit(bool fUseCache=true) const
    {
        if (pwallet == 0)
            return 0;

		int64_t nCredit = 0;

		if (IsDeposit() && GetFirstDepositOutBlocksToMaturity() > 0 && IsInMainChain()) {
			if (fUseCache && fInDepositCreditCached)
				return nInDepositCreditCached;

			//Only first vout is deposit
    		AddTxOutValue(nCredit, GetHash(), 0);

			nInDepositCreditCached = nCredit;
			fInDepositCreditCached = true;
        }

        return nCredit;
    }


    int64_t GetChange(const Credits_CWallet *keyholder_wallet = NULL) const
    {
        // Deposits can never have change
        if (IsDeposit())
            return 0;

        if (fChangeCached)
            return nChangeCached;
        if(keyholder_wallet) {
        	nChangeCached = keyholder_wallet->GetChange(*this);
        } else {
        	nChangeCached = pwallet->GetChange(*this);
        }
        fChangeCached = true;
        return nChangeCached;
    }

    void GetAmounts(std::list<std::pair<CTxDestination, int64_t> >& listReceived,
                    std::list<std::pair<CTxDestination, int64_t> >& listSent, int64_t& nFee, std::string& strSentAccount) const;

    void GetAccountAmounts(const std::string& strAccount, int64_t& nReceived,
                           int64_t& nSent, int64_t& nFee) const;

    bool IsFromMe() const
    {
        return (GetDebit() > 0);
    }

    bool IsTrusted() const
    {
        // Quick answer in most cases
        if (!Credits_IsFinalTx(*this))
            return false;
        int nDepth = GetDepthInMainChain();
        if (nDepth >= 1)
            return true;
        if (nDepth < 0)
            return false;
        if (!credits_bSpendZeroConfChange || !IsFromMe()) // using wtx's cached debit
            return false;

        // Trusted if all inputs are from us and are in the mempool:
        BOOST_FOREACH(const Credits_CTxIn& txin, vin)
        {
            // Transactions not sent by us: not trusted
            const Credits_CWalletTx* parent = pwallet->GetWalletTx(txin.prevout.hash);
            if (parent == NULL)
                return false;
            const CTxOut& parentOut = parent->vout[txin.prevout.n];
            if (!pwallet->IsMine(parentOut))
                return false;
        }
        return true;
    }

    bool WriteToDisk();

    int64_t GetTxTime() const;
    int GetRequestCount() const;

    void RelayWalletTransaction();

    std::set<uint256> GetConflicts() const;
};




class Credits_COutput
{
public:
    const Credits_CWalletTx *tx;
    int i;
    int nDepth;

    Credits_COutput(const Credits_CWalletTx *txIn, int iIn, int nDepthIn)
    {
        tx = txIn; i = iIn; nDepth = nDepthIn;
    }

    std::string ToString() const
    {
        return strprintf("COutput(%s, %d, %d) [%s]", tx->GetHash().ToString(), i, nDepth, FormatMoney(tx->vout[i].nValue).c_str());
    }

    void print() const
    {
        LogPrintf("%s\n", ToString());
    }
};



#endif
