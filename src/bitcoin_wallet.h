// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_BITCOIN_WALLET_H
#define BITCOIN_BITCOIN_WALLET_H

#include "bitcoin_core.h"
#include "crypter.h"
#include "key.h"
#include "keystore.h"
#include "bitcoin_main.h"
#include "ui_interface.h"
#include "util.h"
#include "bitcoin_walletdb.h"

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

// Settings
extern int64_t bitcoin_nTransactionFee;
extern bool bitcoin_bSpendZeroConfChange;

// -paytxfee default
static const int64_t BITCOIN_DEFAULT_TRANSACTION_FEE = 0;
// -paytxfee will warn if called with a higher fee than this amount (in satoshis) per KB
static const int bitcoin_nHighTransactionFeeWarning = 0.01 * COIN;

class CAccountingEntry;
class CCoinControl;
class Bitcoin_COutput;
class Bitcoin_CReserveKey;
class CScript;
class Bitcoin_CWalletTx;

/** (client) version numbers for particular wallet features */
enum Bitcoin_WalletFeature
{
    BITCOIN_FEATURE_BASE = 10500, // the earliest version new wallets supports (only useful for getinfo's clientversion output)

    BITCOIN_FEATURE_WALLETCRYPT = 40000, // wallet encryption
    BITCOIN_FEATURE_COMPRPUBKEY = 60000, // compressed public keys

    BITCOIN_FEATURE_LATEST = 60000
};

//Checks if the passed in transactions vin is within the mapFilterTxInPoints map. Used for filtering out transactions
inline bool IsInFilterPoints(const uint256 &txHash, const int &n, map<uint256, set<int> >& mapFilterTxInPoints) {
	const map<uint256, set<int> >::iterator it = mapFilterTxInPoints.find(txHash);
	if (it != mapFilterTxInPoints.end()) {
		if (it->second.count(n) > 0) {
			return true;
		}
	}
	return false;
}



//Checks if the passed in transactions vin is within the mapFilterTxInPoints map. Used for filtering out transactions that
//for example overlaps with already prepared deposits
inline bool IsAnyTxInInFilterPoints(const Bitcredit_CTransaction& tx,  map<uint256, set<int> >& mapFilterTxInPoints) {
	BOOST_FOREACH (const Bitcredit_CTxIn & ctxIn, tx.vin) {
		const uint256 &hash = ctxIn.prevout.hash;
		const unsigned int &n = ctxIn.prevout.n;

		if(IsInFilterPoints(hash, n, mapFilterTxInPoints)) {
			return true;
		}
	}
	return false;
}

/** A key pool entry */
class CKeyPool
{
public:
    int64_t nTime;
    CPubKey vchPubKey;

    CKeyPool()
    {
        nTime = GetTime();
    }

    CKeyPool(const CPubKey& vchPubKeyIn)
    {
        nTime = GetTime();
        vchPubKey = vchPubKeyIn;
    }

    IMPLEMENT_SERIALIZE
    (
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(nTime);
        READWRITE(vchPubKey);
    )
};

/** Address book data */
class CAddressBookData
{
public:
    std::string name;
    std::string purpose;

    CAddressBookData()
    {
        purpose = "unknown";
    }

    typedef std::map<std::string, std::string> StringMap;
    StringMap destdata;
};

/** A CWallet is an extension of a keystore, which also maintains a set of transactions and balances,
 * and provides the ability to create new transactions.
 */
class Bitcoin_CWallet : public CCryptoKeyStore, public Bitcoin_CWalletInterface
{
private:

    Bitcoin_CWalletDB *pwalletdbEncryption;

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

public:
    bool SelectCoins(int64_t nTargetValue, std::set<std::pair<const Bitcoin_CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet, Bitcoin_CClaimCoinsViewCache& claim_view, map<uint256, set<int> >& mapFilterTxInPoints, const CCoinControl *coinControl = NULL) const;
    /// Main wallet lock.
    /// This lock protects all the fields added by CWallet
    ///   except for:
    ///      fFileBacked (immutable after instantiation)
    ///      strWalletFile (immutable after instantiation)
    mutable CCriticalSection cs_wallet;

    bool fFileBacked;
    std::string strWalletFile;

    std::set<int64_t> setKeyPool;
    std::map<CKeyID, Bitcoin_CKeyMetadata> mapKeyMetadata;

    typedef std::map<unsigned int, CMasterKey> MasterKeyMap;
    MasterKeyMap mapMasterKeys;
    unsigned int nMasterKeyMaxID;

    Bitcoin_CWallet()
    {
        SetNull();
    }
    Bitcoin_CWallet(std::string strWalletFileIn)
    {
        SetNull();

        strWalletFile = strWalletFileIn;
        fFileBacked = true;
    }
    void SetNull()
    {
        nWalletVersion = BITCOIN_FEATURE_BASE;
        nWalletMaxVersion = BITCOIN_FEATURE_BASE;
        fFileBacked = false;
        nMasterKeyMaxID = 0;
        pwalletdbEncryption = NULL;
        nOrderPosNext = 0;
        nNextResend = 0;
        nLastResend = 0;
        nTimeFirstKey = 0;
    }

    std::map<uint256, Bitcoin_CWalletTx> mapWallet;

    int64_t nOrderPosNext;
    std::map<uint256, int> mapRequestCount;

    std::map<CTxDestination, CAddressBookData> mapAddressBook;

    CPubKey vchDefaultKey;

    std::set<COutPoint> setLockedCoins;

    int64_t nTimeFirstKey;

    const Bitcoin_CWalletTx* GetWalletTx(const uint256& hash) const;

    // check whether we are allowed to upgrade (or already support) to the named feature
    bool CanSupportFeature(enum Bitcoin_WalletFeature wf) { AssertLockHeld(cs_wallet); return nWalletMaxVersion >= wf; }

    int GetBestBlockClaimDepth(Bitcoin_CClaimCoinsViewCache& claim_view) const;

    void AvailableCoins(std::vector<Bitcoin_COutput>& vCoins, Bitcoin_CClaimCoinsViewCache& claim_view, map<uint256, set<int> >& mapFilterTxPoints, bool fOnlyConfirmed=true, const CCoinControl *coinControl = NULL) const;
    bool SelectCoinsMinConf(Bitcoin_CClaimCoinsViewCache& claim_view, int64_t nTargetValue, int nConfMine, int nConfTheirs, std::vector<Bitcoin_COutput> vCoins, std::set<std::pair<const Bitcoin_CWalletTx*,unsigned int> >& setCoinsRet, int64_t& nValueRet) const;

    bool IsSpent(const uint256& hash, unsigned int n, unsigned int claimBestBlockDepth) const;

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
    bool LoadKeyMetadata(const CPubKey &pubkey, const Bitcoin_CKeyMetadata &metadata);

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
    int64_t IncOrderPosNext(Bitcoin_CWalletDB *pwalletdb = NULL);

    typedef std::pair<Bitcoin_CWalletTx*, CAccountingEntry*> TxPair;
    typedef std::multimap<int64_t, TxPair > TxItems;

    /** Get the wallet's activity log
        @return multimap of ordered transactions and accounting entries
        @warning Returned pointers are *only* valid within the scope of passed acentries
     */
    TxItems OrderedTxItems(std::list<CAccountingEntry>& acentries, std::string strAccount = "");

    void MarkDirty();
    bool AddToWallet(const Bitcoin_CWalletTx& wtxIn, bool fFromLoadWallet=false);
    void SyncTransaction(Bitcoin_CClaimCoinsViewCache& claim_view, const uint256 &hash, const Bitcoin_CTransaction& tx, const Bitcoin_CBlock* pblock);
    bool AddToWalletIfInvolvingMe(Bitcoin_CClaimCoinsViewCache& claim_view, const uint256 &hash, const Bitcoin_CTransaction& tx, const Bitcoin_CBlock* pblock, bool fUpdate);
    void EraseFromWallet(const uint256 &hash);
    int ScanForWalletTransactions(Bitcoin_CClaimCoinsViewCache& claim_view, Bitcoin_CBlockIndex* pindexStart, Bitcoin_CBlockIndex* pindexStop, bool fUpdate = false);
    void ReacceptWalletTransactions();
//    void ResendWalletTransactions();
    int64_t GetBalance(Bitcoin_CClaimCoinsViewCache& claim_view, map<uint256, set<int> >& mapFilterTxInPoints) const;
    int64_t GetUnconfirmedBalance(Bitcoin_CClaimCoinsViewCache& claim_view, map<uint256, set<int> >& mapFilterTxInPoints) const;
    int64_t GetImmatureBalance(Bitcoin_CClaimCoinsViewCache& claim_view, map<uint256, set<int> >& mapFilterTxInPoints) const;
//    bool CreateTransaction(const std::vector<std::pair<CScript, int64_t> >& vecSend,
//                           Bitcoin_CWalletTx& wtxNew, Bitcoin_CReserveKey& reservekey, int64_t& nFeeRet, std::string& strFailReason, const CCoinControl *coinControl = NULL);
//    bool CreateTransaction(CScript scriptPubKey, int64_t nValue,
//                           Bitcoin_CWalletTx& wtxNew, Bitcoin_CReserveKey& reservekey, int64_t& nFeeRet, std::string& strFailReason, const CCoinControl *coinControl = NULL);
//    bool CommitTransaction(Bitcoin_CWalletTx& wtxNew, Bitcoin_CReserveKey& reservekey);
//    std::string SendMoney(CScript scriptPubKey, int64_t nValue, Bitcoin_CWalletTx& wtxNew);
//    std::string SendMoneyToDestination(const CTxDestination &address, int64_t nValue, Bitcoin_CWalletTx& wtxNew);

    bool NewKeyPool();
    bool TopUpKeyPool(unsigned int kpSize = 0);
    void ReserveKeyFromKeyPool(int64_t& nIndex, CKeyPool& keypool);
    void KeepKey(int64_t nIndex);
    void ReturnKey(int64_t nIndex);
    bool GetKeyFromPool(CPubKey &key);
    int64_t GetOldestKeyPoolTime();
    void GetAllReserveKeys(std::set<CKeyID>& setAddress) const;

    std::set< std::set<CTxDestination> > GetAddressGroupings();
    std::map<CTxDestination, int64_t> GetAddressBalances(Bitcoin_CClaimCoinsViewCache& claim_view);

    std::set<CTxDestination> GetAccountAddresses(std::string strAccount) const;

    bool IsMine(const Bitcoin_CTxIn& txin) const;
    int64_t GetDebit(const Bitcoin_CTxIn& txin, Bitcoin_CClaimCoinsViewCache &claim_view) const;
    int64_t GetDebitWithoutClaimed(const Bitcoin_CTxIn& txin) const;
    int64_t GetDebit(const Bitcredit_CTxIn& txin, Bitcoin_CClaimCoinsViewCache &claim_view) const;
    bool IsMine(const CTxOut& txout) const
    {
        return ::IsMine(*this, txout.scriptPubKey);
    }
    int64_t GetCredit(const CTxOut& txout, int64_t nValue) const
    {
    	assert_with_stacktrace(Bitcoin_MoneyRange(nValue), strprintf("Bitcoin: CWallet::GetCredit() : value out of range: %d:%d", txout.nValue, nValue));
        return (IsMine(txout) ? nValue : 0);
    }
    bool IsChange(const CTxOut& txout) const;
    int64_t GetChange(const CTxOut& txout, int64_t nValue) const
    {
    	assert_with_stacktrace(Bitcoin_MoneyRange(nValue), "Bitcoin: CWallet::GetChange() : value out of range");
        return (IsChange(txout) ? nValue : 0);
    }
    bool IsMine(const Bitcoin_CTransaction& tx) const
    {
        BOOST_FOREACH(const CTxOut& txout, tx.vout)
            if (IsMine(txout))
                return true;
        return false;
    }
    bool IsFromMe(const Bitcoin_CTransaction& tx, Bitcoin_CClaimCoinsViewCache &claim_view) const
    {
        return (GetDebit(tx, claim_view) > 0);
    }
    bool IsFromMe(const Bitcredit_CTransaction& tx, Bitcoin_CClaimCoinsViewCache &claim_view) const
    {
        return (GetDebit(tx, claim_view) > 0);
    }
    int64_t GetDebit(const Bitcoin_CTransaction& tx, Bitcoin_CClaimCoinsViewCache &claim_view) const
    {
        int64_t nDebit = 0;
        BOOST_FOREACH(const Bitcoin_CTxIn& txin, tx.vin)
        {
            nDebit += GetDebit(txin, claim_view);
            assert_with_stacktrace(Bitcoin_MoneyRange(nDebit), "Bitcoin: CWallet::GetDebit() : value out of range");
        }
        return nDebit;
    }
    int64_t GetDebitWithoutClaimed(const Bitcoin_CTransaction& tx) const
    {
        int64_t nDebit = 0;
        BOOST_FOREACH(const Bitcoin_CTxIn& txin, tx.vin)
        {
            nDebit += GetDebitWithoutClaimed(txin);
            assert_with_stacktrace(Bitcoin_MoneyRange(nDebit), "Bitcoin: CWallet::GetDebit() : value out of range");
        }
        return nDebit;
    }
    int64_t GetDebit(const Bitcredit_CTransaction& tx, Bitcoin_CClaimCoinsViewCache &claim_view) const
    {
        int64_t nDebit = 0;
        BOOST_FOREACH(const Bitcredit_CTxIn& txin, tx.vin)
        {
            nDebit += GetDebit(txin, claim_view);
            assert_with_stacktrace(Bitcoin_MoneyRange(nDebit), "Bitcoin: CWallet::GetDebit() : value out of range");
        }
        return nDebit;
    }

    int64_t GetCredit(const Bitcoin_CTransaction& tx, Bitcoin_CClaimCoinsViewCache& claim_view, map<uint256, set<int> >& mapFilterTxInPoints) const
    {
    	const uint256 &hashTx = tx.GetHash();
        int64_t nCredit = 0;
        if(claim_view.HaveCoins(hashTx)) {
			const Bitcoin_CClaimCoins & claimCoins = claim_view.GetCoins(hashTx);

			for(unsigned int i = 0; i < tx.vout.size(); i++) {
				if(!IsInFilterPoints(hashTx, i, mapFilterTxInPoints)) {
					if(claimCoins.HasClaimable(i)) {
						const CTxOut& txout = tx.vout[i];
						const CTxOutClaim &txoutClaim = claimCoins.vout[i];

						assert_with_stacktrace(txout.nValue == txoutClaim.nValueOriginal, strprintf("CWallet:GetCreditForTx() : value not equal to valueOriginal: %d:%d, \nTx:\n%s, \nclaimcoins:\n%s", txout.nValue, txoutClaim.nValueOriginal, tx.ToString(), claimCoins.ToString()));
						assert_with_stacktrace(Bitcoin_MoneyRange(txoutClaim.nValueClaimable), strprintf("CWallet:GetCreditForTx() : value out of range: %d:%d, Hash: %s", txout.nValue, txoutClaim.nValueClaimable, hashTx.ToString()));

						nCredit += GetCredit(txout, txoutClaim.nValueClaimable);

						assert_with_stacktrace(Bitcoin_MoneyRange(nCredit), "Bitcoin: CWallet::GetCreditForTx() : value out of range");
					}
				}
			}
        }
        return nCredit;
    }
    int64_t GetChange(const Bitcoin_CTransaction& tx, Bitcoin_CClaimCoinsViewCache& claim_view) const
    {
    	const uint256 &hashTx = tx.GetHash();
        int64_t nChange = 0;
        if(claim_view.HaveCoins(hashTx)) {
			const Bitcoin_CClaimCoins & claimCoins = claim_view.GetCoins(hashTx);

			for(unsigned int i = 0; i < tx.vout.size(); i++) {
				if(claimCoins.HasClaimable(i)) {
					const CTxOut& txout = tx.vout[i];

					nChange += GetChange(txout, claimCoins.vout[i].nValueClaimable);
					assert_with_stacktrace(Bitcoin_MoneyRange(nChange), "Bitcoin: CWallet::GetChange() : value out of range");
				}
			}
        }
        return nChange;
    }
    void SetBestChain(const CBlockLocator& loc);

    Bitcoin_DBErrors LoadWallet(bool& fFirstRunRet);
    Bitcoin_DBErrors ZapWalletTx();

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
    bool SetMinVersion(enum Bitcoin_WalletFeature, Bitcoin_CWalletDB* pwalletdbIn = NULL, bool fExplicit = false);

    // change which version we're allowed to upgrade to (note that this does not immediately imply upgrading to that format)
    bool SetMaxVersion(int nVersion);

    // get the current wallet format (the oldest client version guaranteed to understand this wallet)
    int GetVersion() { LOCK(cs_wallet); return nWalletVersion; }

    // Get wallet transactions that conflict with given transaction (spend same outputs)
    std::set<uint256> GetConflicts(const uint256& txid) const;

    /** Address book entry changed.
     * @note called with lock cs_wallet held.
     */
    boost::signals2::signal<void (Bitcoin_CWallet *wallet, const CTxDestination
            &address, const std::string &label, bool isMine,
            const std::string &purpose,
            ChangeType status)> NotifyAddressBookChanged;

    /** Wallet transaction added, removed or updated.
     * @note called with lock cs_wallet held.
     */
    boost::signals2::signal<void (Bitcoin_CWallet *wallet, const uint256 &hashTx,
            ChangeType status)> NotifyTransactionChanged;

    /** Show progress e.g. for rescan */
    boost::signals2::signal<void (const std::string &title, int nProgress)> ShowProgress;
};

/** A key allocated from the key pool. */
class Bitcoin_CReserveKey
{
protected:
    Bitcoin_CWallet* pwallet;
    int64_t nIndex;
    CPubKey vchPubKey;
public:
    Bitcoin_CReserveKey(Bitcoin_CWallet* pwalletIn)
    {
        nIndex = -1;
        pwallet = pwalletIn;
    }

    ~Bitcoin_CReserveKey()
    {
        ReturnKey();
    }

    void ReturnKey();
    bool GetReservedKey(CPubKey &pubkey);
    void KeepKey();
};


typedef std::map<std::string, std::string> mapValue_t;


static void ReadOrderPos(int64_t& nOrderPos, mapValue_t& mapValue)
{
    if (!mapValue.count("n"))
    {
        nOrderPos = -1; // TODO: calculate elsewhere
        return;
    }
    nOrderPos = atoi64(mapValue["n"].c_str());
}


static void WriteOrderPos(const int64_t& nOrderPos, mapValue_t& mapValue)
{
    if (nOrderPos == -1)
        return;
    mapValue["n"] = i64tostr(nOrderPos);
}

/** A transaction with a bunch of additional info that only the owner cares about.
 * It includes any unrecorded transactions needed to link it back to the block chain.
 */
class Bitcoin_CWalletTx : public Bitcoin_CMerkleTx
{
private:
    const Bitcoin_CWallet* pwallet;

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
    mutable bool fDebitCachedWithoutClaimed;
    mutable bool fCreditCached;
    mutable bool fImmatureCreditCached;
    mutable bool fAvailableCreditCached;
    mutable bool fChangeCached;
    mutable int64_t nDebitCached;
    mutable int64_t nDebitCachedWithoutClaimed;
    mutable int64_t nCreditCached;
    mutable int64_t nImmatureCreditCached;
    mutable int64_t nAvailableCreditCached;
    mutable int64_t nChangeCached;

    Bitcoin_CWalletTx()
    {
        Init(NULL);
    }

    Bitcoin_CWalletTx(const Bitcoin_CWallet* pwalletIn)
    {
        Init(pwalletIn);
    }

    Bitcoin_CWalletTx(const Bitcoin_CWallet* pwalletIn, const Bitcoin_CMerkleTx& txIn) : Bitcoin_CMerkleTx(txIn)
    {
        Init(pwalletIn);
    }

    Bitcoin_CWalletTx(const Bitcoin_CWallet* pwalletIn, const Bitcoin_CTransaction& txIn) : Bitcoin_CMerkleTx(txIn)
    {
        Init(pwalletIn);
    }

    void Init(const Bitcoin_CWallet* pwalletIn)
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
        fDebitCachedWithoutClaimed = false;
        fCreditCached = false;
        fImmatureCreditCached = false;
        fAvailableCreditCached = false;
        fChangeCached = false;
        nDebitCached = 0;
        nDebitCachedWithoutClaimed = 0;
        nCreditCached = 0;
        nImmatureCreditCached = 0;
        nAvailableCreditCached = 0;
        nChangeCached = 0;
        nOrderPos = -1;
    }

    IMPLEMENT_SERIALIZE
    (
        Bitcoin_CWalletTx* pthis = const_cast<Bitcoin_CWalletTx*>(this);
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

        nSerSize += SerReadWrite(s, *(Bitcoin_CMerkleTx*)this, nType, nVersion,ser_action);
        std::vector<Bitcoin_CMerkleTx> vUnused; // Used to be vtxPrev
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
        fDebitCached = false;
        fDebitCachedWithoutClaimed = false;
        fChangeCached = false;
    }

    void BindWallet(Bitcoin_CWallet *pwalletIn)
    {
        pwallet = pwalletIn;
        MarkDirty();
    }

    int64_t GetDebit(Bitcoin_CClaimCoinsViewCache& claim_view) const
    {
        if (vin.empty())
            return 0;
        if (fDebitCached)
            return nDebitCached;
        nDebitCached = pwallet->GetDebit(*this, claim_view);
        fDebitCached = true;
        return nDebitCached;
    }

    int64_t GetDebitWithoutClaimed() const
    {
        if (vin.empty())
            return 0;
        if (fDebitCachedWithoutClaimed)
            return nDebitCachedWithoutClaimed;
        nDebitCachedWithoutClaimed = pwallet->GetDebitWithoutClaimed(*this);
        fDebitCachedWithoutClaimed = true;
        return nDebitCachedWithoutClaimed;
    }

    int64_t GetCredit(Bitcoin_CClaimCoinsViewCache& claim_view, map<uint256, set<int> >& mapFilterTxInPoints, bool fUseCache=true) const
    {
        // Must wait until coinbase is safely deep enough in the chain before valuing it
        if (IsCoinBase() && GetBlocksToMaturity() > 0)
            return 0;

        // GetBalance can assume transactions in mapWallet won't change
        if (fUseCache && fCreditCached)
            return nCreditCached;
        nCreditCached = pwallet->GetCredit(*this, claim_view, mapFilterTxInPoints);
        fCreditCached = true;
        return nCreditCached;
    }

    int64_t GetImmatureCredit(Bitcoin_CClaimCoinsViewCache& claim_view, map<uint256, set<int> >& mapFilterTxInPoints, bool fUseCache=true) const
    {
        if (IsCoinBase() && GetBlocksToMaturity() > 0 && IsInMainChain())
        {
            if (fUseCache && fImmatureCreditCached)
                return nImmatureCreditCached;
            nImmatureCreditCached = pwallet->GetCredit(*this, claim_view, mapFilterTxInPoints);
            fImmatureCreditCached = true;
            return nImmatureCreditCached;
        }

        return 0;
    }

    int64_t GetAvailableCredit(Bitcoin_CClaimCoinsViewCache& claim_view, map<uint256, set<int> >& mapFilterTxInPoints, bool fUseCache=true) const
    {
        if (pwallet == 0)
            return 0;

        // Must wait until coinbase is safely deep enough in the chain before valuing it
        if (IsCoinBase() && GetBlocksToMaturity() > 0)
            return 0;

        if (fUseCache && fAvailableCreditCached)
            return nAvailableCreditCached;

		const int nClaimBestBlockDepth = pwallet->GetBestBlockClaimDepth(claim_view);

        int64_t nCredit = 0;
        const uint256 &hashTx = GetHash();
        if(claim_view.HaveCoins(hashTx)) {
			const Bitcoin_CClaimCoins & claimCoins = claim_view.GetCoins(hashTx);

			for(unsigned int i = 0; i < vout.size(); i++) {
	    		if(!IsInFilterPoints(hashTx, i, mapFilterTxInPoints)) {
					if(claimCoins.HasClaimable(i)) {
						if (!pwallet->IsSpent(hashTx, i, nClaimBestBlockDepth))
						{
							const CTxOut &txout = vout[i];
							const CTxOutClaim &txoutClaim = claimCoins.vout[i];

							assert_with_stacktrace(Bitcoin_MoneyRange(txoutClaim.nValueClaimable), strprintf("CWalletTx:GetAvailableCredit() : value out of range: %d:%d, Hash: %s", txout.nValue, txoutClaim.nValueClaimable, hashTx.ToString()));
							nCredit += pwallet->GetCredit(txout, txoutClaim.nValueClaimable);
							assert_with_stacktrace(Bitcoin_MoneyRange(nCredit), "Bitcoin: CWalletTx::GetAvailableCredit() : value out of range");
						}
					}
	    		}
			}
        }

        nAvailableCreditCached = nCredit;
        fAvailableCreditCached = true;
        return nCredit;
    }


    int64_t GetChange(Bitcoin_CClaimCoinsViewCache& claim_view) const
    {
        if (fChangeCached)
            return nChangeCached;
        nChangeCached = pwallet->GetChange(*this, claim_view);
        fChangeCached = true;
        return nChangeCached;
    }

    void GetAmounts(Bitcoin_CClaimCoinsViewCache& claim_view, std::list<std::pair<CTxDestination, int64_t> >& listReceived,
                    std::list<std::pair<CTxDestination, int64_t> >& listSent, int64_t& nFee, std::string& strSentAccount) const;

    void GetAccountAmounts(Bitcoin_CClaimCoinsViewCache& claim_view, const std::string& strAccount, int64_t& nReceived,
                           int64_t& nSent, int64_t& nFee) const;

    bool IsFromMe(Bitcoin_CClaimCoinsViewCache& claim_view) const
    {
        return (GetDebit(claim_view) > 0);
    }

    bool IsTrusted(Bitcoin_CClaimCoinsViewCache& claim_view) const
    {
        // Quick answer in most cases
        if (!Bitcoin_IsFinalTx(*this))
            return false;
        int nDepth = GetDepthInMainChain();
        if (nDepth >= 1)
            return true;
        if (nDepth < 0)
            return false;
        if (!bitcoin_bSpendZeroConfChange || !IsFromMe(claim_view)) // using wtx's cached debit
            return false;

        // Trusted if all inputs are from us and are in the mempool:
        BOOST_FOREACH(const Bitcoin_CTxIn& txin, vin)
        {
            // Transactions not sent by us: not trusted
            const Bitcoin_CWalletTx* parent = pwallet->GetWalletTx(txin.prevout.hash);
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

//    void RelayWalletTransaction();

    std::set<uint256> GetConflicts() const;
};




class Bitcoin_COutput
{
public:
    const Bitcoin_CWalletTx *tx;
    int i;
    int nDepth;
    int64_t nValue;

    Bitcoin_COutput(const Bitcoin_CWalletTx *txIn, int iIn, int nDepthIn, int64_t nValueIn)
    {
        tx = txIn; i = iIn; nDepth = nDepthIn; nValue = nValueIn;
    }

    std::string ToString() const
    {
        return strprintf("COutput(%s, %d, %d) [%s]:[%s]", tx->GetHash().ToString(), i, nDepth, FormatMoney(tx->vout[i].nValue).c_str(), FormatMoney(nValue).c_str());
    }

    void print() const
    {
        LogPrintf("%s\n", ToString());
    }
};


/** Private key that includes an expiration date in case it never gets used. */
class CWalletKey
{
public:
    CPrivKey vchPrivKey;
    int64_t nTimeCreated;
    int64_t nTimeExpires;
    std::string strComment;
    //// todo: add something to note what created it (user, getnewaddress, change)
    ////   maybe should have a map<string, string> property map

    CWalletKey(int64_t nExpires=0)
    {
        nTimeCreated = (nExpires ? GetTime() : 0);
        nTimeExpires = nExpires;
    }

    IMPLEMENT_SERIALIZE
    (
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vchPrivKey);
        READWRITE(nTimeCreated);
        READWRITE(nTimeExpires);
        READWRITE(strComment);
    )
};






/** Account information.
 * Stored in wallet with key "acc"+string account name.
 */
class CAccount
{
public:
    CPubKey vchPubKey;

    CAccount()
    {
        SetNull();
    }

    void SetNull()
    {
        vchPubKey = CPubKey();
    }

    IMPLEMENT_SERIALIZE
    (
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vchPubKey);
    )
};



/** Internal transfers.
 * Database key is acentry<account><counter>.
 */
class CAccountingEntry
{
public:
    std::string strAccount;
    int64_t nCreditDebit;
    int64_t nTime;
    std::string strOtherAccount;
    std::string strComment;
    mapValue_t mapValue;
    int64_t nOrderPos;  // position in ordered transaction list
    uint64_t nEntryNo;

    CAccountingEntry()
    {
        SetNull();
    }

    void SetNull()
    {
        nCreditDebit = 0;
        nTime = 0;
        strAccount.clear();
        strOtherAccount.clear();
        strComment.clear();
        nOrderPos = -1;
    }

    IMPLEMENT_SERIALIZE
    (
        CAccountingEntry& me = *const_cast<CAccountingEntry*>(this);
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        // Note: strAccount is serialized as part of the key, not here.
        READWRITE(nCreditDebit);
        READWRITE(nTime);
        READWRITE(strOtherAccount);

        if (!fRead)
        {
            WriteOrderPos(nOrderPos, me.mapValue);

            if (!(mapValue.empty() && _ssExtra.empty()))
            {
                CDataStream ss(nType, nVersion);
                ss.insert(ss.begin(), '\0');
                ss << mapValue;
                ss.insert(ss.end(), _ssExtra.begin(), _ssExtra.end());
                me.strComment.append(ss.str());
            }
        }

        READWRITE(strComment);

        size_t nSepPos = strComment.find("\0", 0, 1);
        if (fRead)
        {
            me.mapValue.clear();
            if (std::string::npos != nSepPos)
            {
                CDataStream ss(std::vector<char>(strComment.begin() + nSepPos + 1, strComment.end()), nType, nVersion);
                ss >> me.mapValue;
                me._ssExtra = std::vector<char>(ss.begin(), ss.end());
            }
            ReadOrderPos(me.nOrderPos, me.mapValue);
        }
        if (std::string::npos != nSepPos)
            me.strComment.erase(nSepPos);

        me.mapValue.erase("n");
    )

private:
    std::vector<char> _ssExtra;
};



#endif
