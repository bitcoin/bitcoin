// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2011 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_WALLET_H
#define BITCOIN_WALLET_H

#include "bignum.h"
#include "key.h"
#include "script.h"

class CWalletTx;
class CReserveKey;
class CWalletDB;

class CWallet : public CCryptoKeyStore
{
private:
    bool SelectCoinsMinConf(int64 nTargetValue, int nConfMine, int nConfTheirs, std::set<std::pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64& nValueRet) const;
    bool SelectCoins(int64 nTargetValue, std::set<std::pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64& nValueRet) const;

    CWalletDB *pwalletdbEncryption;

public:
    mutable CCriticalSection cs_wallet;

    bool fFileBacked;
    std::string strWalletFile;

    std::set<int64> setKeyPool;

    typedef std::map<unsigned int, CMasterKey> MasterKeyMap;
    MasterKeyMap mapMasterKeys;
    unsigned int nMasterKeyMaxID;

    CWallet()
    {
        fFileBacked = false;
        nMasterKeyMaxID = 0;
        pwalletdbEncryption = NULL;
    }
    CWallet(std::string strWalletFileIn)
    {
        strWalletFile = strWalletFileIn;
        fFileBacked = true;
        nMasterKeyMaxID = 0;
        pwalletdbEncryption = NULL;
    }

    std::map<uint256, CWalletTx> mapWallet;
    std::vector<uint256> vWalletUpdated;

    std::map<uint256, int> mapRequestCount;

    std::map<CBitcoinAddress, std::string> mapAddressBook;

    std::vector<unsigned char> vchDefaultKey;

    // keystore implementation
    bool AddKey(const CKey& key);
    bool LoadKey(const CKey& key) { return CCryptoKeyStore::AddKey(key); }
    bool AddCryptedKey(const std::vector<unsigned char> &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);
    bool LoadCryptedKey(const std::vector<unsigned char> &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret) { return CCryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret); }

    bool Unlock(const std::string& strWalletPassphrase);
    bool ChangeWalletPassphrase(const std::string& strOldWalletPassphrase, const std::string& strNewWalletPassphrase);
    bool EncryptWallet(const std::string& strWalletPassphrase);

    bool AddToWallet(const CWalletTx& wtxIn);
    bool AddToWalletIfInvolvingMe(const CTransaction& tx, const CBlock* pblock, bool fUpdate = false);
    bool EraseFromWallet(uint256 hash);
    void WalletUpdateSpent(const CTransaction& prevout);
    int ScanForWalletTransactions(CBlockIndex* pindexStart, bool fUpdate = false);
    void ReacceptWalletTransactions();
    void ResendWalletTransactions();
    int64 GetBalance() const;
    int64 GetUnconfirmedBalance() const;
    bool CreateTransaction(const std::vector<std::pair<CScript, int64> >& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, int64& nFeeRet);
    bool CreateTransaction(CScript scriptPubKey, int64 nValue, CWalletTx& wtxNew, CReserveKey& reservekey, int64& nFeeRet);
    bool CommitTransaction(CWalletTx& wtxNew, CReserveKey& reservekey);
    bool BroadcastTransaction(CWalletTx& wtxNew);
    std::string SendMoney(CScript scriptPubKey, int64 nValue, CWalletTx& wtxNew, bool fAskFee=false);
    std::string SendMoneyToBitcoinAddress(const CBitcoinAddress& address, int64 nValue, CWalletTx& wtxNew, bool fAskFee=false);

    bool TopUpKeyPool();
    void ReserveKeyFromKeyPool(int64& nIndex, CKeyPool& keypool);
    void KeepKey(int64 nIndex);
    void ReturnKey(int64 nIndex);
    bool GetKeyFromPool(std::vector<unsigned char> &key, bool fAllowReuse=true);
    int64 GetOldestKeyPoolTime();

    bool IsMine(const CTxIn& txin) const;
    int64 GetDebit(const CTxIn& txin) const;
    bool IsMine(const CTxOut& txout) const
    {
        return ::IsMine(*this, txout.scriptPubKey);
    }
    int64 GetCredit(const CTxOut& txout) const
    {
        if (!MoneyRange(txout.nValue))
            throw std::runtime_error("CWallet::GetCredit() : value out of range");
        return (IsMine(txout) ? txout.nValue : 0);
    }
    bool IsChange(const CTxOut& txout) const
    {
        CBitcoinAddress address;
        if (ExtractAddress(txout.scriptPubKey, this, address))
            CRITICAL_BLOCK(cs_wallet)
                if (!mapAddressBook.count(address))
                    return true;
        return false;
    }
    int64 GetChange(const CTxOut& txout) const
    {
        if (!MoneyRange(txout.nValue))
            throw std::runtime_error("CWallet::GetChange() : value out of range");
        return (IsChange(txout) ? txout.nValue : 0);
    }
    bool IsMine(const CTransaction& tx) const
    {
        BOOST_FOREACH(const CTxOut& txout, tx.vout)
            if (IsMine(txout))
                return true;
        return false;
    }
    bool IsFromMe(const CTransaction& tx) const
    {
        return (GetDebit(tx) > 0);
    }
    int64 GetDebit(const CTransaction& tx) const
    {
        int64 nDebit = 0;
        BOOST_FOREACH(const CTxIn& txin, tx.vin)
        {
            nDebit += GetDebit(txin);
            if (!MoneyRange(nDebit))
                throw std::runtime_error("CWallet::GetDebit() : value out of range");
        }
        return nDebit;
    }
    int64 GetCredit(const CTransaction& tx) const
    {
        int64 nCredit = 0;
        BOOST_FOREACH(const CTxOut& txout, tx.vout)
        {
            nCredit += GetCredit(txout);
            if (!MoneyRange(nCredit))
                throw std::runtime_error("CWallet::GetCredit() : value out of range");
        }
        return nCredit;
    }
    int64 GetChange(const CTransaction& tx) const
    {
        int64 nChange = 0;
        BOOST_FOREACH(const CTxOut& txout, tx.vout)
        {
            nChange += GetChange(txout);
            if (!MoneyRange(nChange))
                throw std::runtime_error("CWallet::GetChange() : value out of range");
        }
        return nChange;
    }
    void SetBestChain(const CBlockLocator& loc)
    {
        CWalletDB walletdb(strWalletFile);
        walletdb.WriteBestBlock(loc);
    }

    int LoadWallet(bool& fFirstRunRet);
//    bool BackupWallet(const std::string& strDest);

    bool SetAddressBookName(const CBitcoinAddress& address, const std::string& strName);

    bool DelAddressBookName(const CBitcoinAddress& address);

    void UpdatedTransaction(const uint256 &hashTx)
    {
        CRITICAL_BLOCK(cs_wallet)
            vWalletUpdated.push_back(hashTx);
    }

    void PrintWallet(const CBlock& block);

    void Inventory(const uint256 &hash)
    {
        CRITICAL_BLOCK(cs_wallet)
        {
            std::map<uint256, int>::iterator mi = mapRequestCount.find(hash);
            if (mi != mapRequestCount.end())
                (*mi).second++;
        }
    }

    int GetKeyPoolSize()
    {
        return setKeyPool.size();
    }

    bool GetTransaction(const uint256 &hashTx, CWalletTx& wtx);

    bool SetDefaultKey(const std::vector<unsigned char> &vchPubKey);
};


class CReserveKey
{
protected:
    CWallet* pwallet;
    int64 nIndex;
    std::vector<unsigned char> vchPubKey;
public:
    CReserveKey(CWallet* pwalletIn)
    {
        nIndex = -1;
        pwallet = pwalletIn;
    }

    ~CReserveKey()
    {
        if (!fShutdown)
            ReturnKey();
    }

    void ReturnKey();
    std::vector<unsigned char> GetReservedKey();
    void KeepKey();
};


//
// A transaction with a bunch of additional info that only the owner cares
// about.  It includes any unrecorded transactions needed to link it back
// to the block chain.
//
class CWalletTx : public CMerkleTx
{
public:
    const CWallet* pwallet;

    std::vector<CMerkleTx> vtxPrev;
    std::map<std::string, std::string> mapValue;
    std::vector<std::pair<std::string, std::string> > vOrderForm;
    unsigned int fTimeReceivedIsTxTime;
    unsigned int nTimeReceived;  // time received by this node
    char fFromMe;
    std::string strFromAccount;
    std::vector<char> vfSpent;

    // memory only
    mutable char fDebitCached;
    mutable char fCreditCached;
    mutable char fAvailableCreditCached;
    mutable char fChangeCached;
    mutable int64 nDebitCached;
    mutable int64 nCreditCached;
    mutable int64 nAvailableCreditCached;
    mutable int64 nChangeCached;

    // memory only UI hints
    mutable unsigned int nTimeDisplayed;
    mutable int nLinesDisplayed;
    mutable char fConfirmedDisplayed;

    CWalletTx()
    {
        Init(NULL);
    }

    CWalletTx(const CWallet* pwalletIn)
    {
        Init(pwalletIn);
    }

    CWalletTx(const CWallet* pwalletIn, const CMerkleTx& txIn) : CMerkleTx(txIn)
    {
        Init(pwalletIn);
    }

    CWalletTx(const CWallet* pwalletIn, const CTransaction& txIn) : CMerkleTx(txIn)
    {
        Init(pwalletIn);
    }

    void Init(const CWallet* pwalletIn)
    {
        pwallet = pwalletIn;
        vtxPrev.clear();
        mapValue.clear();
        vOrderForm.clear();
        fTimeReceivedIsTxTime = false;
        nTimeReceived = 0;
        fFromMe = false;
        strFromAccount.clear();
        vfSpent.clear();
        fDebitCached = false;
        fCreditCached = false;
        fAvailableCreditCached = false;
        fChangeCached = false;
        nDebitCached = 0;
        nCreditCached = 0;
        nAvailableCreditCached = 0;
        nChangeCached = 0;
        nTimeDisplayed = 0;
        nLinesDisplayed = 0;
        fConfirmedDisplayed = false;
    }

    IMPLEMENT_SERIALIZE
    (
        CWalletTx* pthis = const_cast<CWalletTx*>(this);
        if (fRead)
            pthis->Init(NULL);
        char fSpent = false;

        if (!fRead)
        {
            pthis->mapValue["fromaccount"] = pthis->strFromAccount;

            std::string str;
            BOOST_FOREACH(char f, vfSpent)
            {
                str += (f ? '1' : '0');
                if (f)
                    fSpent = true;
            }
            pthis->mapValue["spent"] = str;
        }

        nSerSize += SerReadWrite(s, *(CMerkleTx*)this, nType, nVersion,ser_action);
        READWRITE(vtxPrev);
        READWRITE(mapValue);
        READWRITE(vOrderForm);
        READWRITE(fTimeReceivedIsTxTime);
        READWRITE(nTimeReceived);
        READWRITE(fFromMe);
        READWRITE(fSpent);

        if (fRead)
        {
            pthis->strFromAccount = pthis->mapValue["fromaccount"];

            if (mapValue.count("spent"))
                BOOST_FOREACH(char c, pthis->mapValue["spent"])
                    pthis->vfSpent.push_back(c != '0');
            else
                pthis->vfSpent.assign(vout.size(), fSpent);
        }

        pthis->mapValue.erase("fromaccount");
        pthis->mapValue.erase("version");
        pthis->mapValue.erase("spent");
    )

    // marks certain txout's as spent
    // returns true if any update took place
    bool UpdateSpent(const std::vector<char>& vfNewSpent)
    {
        bool fReturn = false;
        for (int i=0; i < vfNewSpent.size(); i++)
        {
            if (i == vfSpent.size())
                break;

            if (vfNewSpent[i] && !vfSpent[i])
            {
                vfSpent[i] = true;
                fReturn = true;
                fAvailableCreditCached = false;
            }
        }
        return fReturn;
    }

    void MarkDirty()
    {
        fCreditCached = false;
        fAvailableCreditCached = false;
        fDebitCached = false;
        fChangeCached = false;
    }

    void MarkSpent(unsigned int nOut)
    {
        if (nOut >= vout.size())
            throw std::runtime_error("CWalletTx::MarkSpent() : nOut out of range");
        vfSpent.resize(vout.size());
        if (!vfSpent[nOut])
        {
            vfSpent[nOut] = true;
            fAvailableCreditCached = false;
        }
    }

    bool IsSpent(unsigned int nOut) const
    {
        if (nOut >= vout.size())
            throw std::runtime_error("CWalletTx::IsSpent() : nOut out of range");
        if (nOut >= vfSpent.size())
            return false;
        return (!!vfSpent[nOut]);
    }

    int64 GetDebit() const
    {
        if (vin.empty())
            return 0;
        if (fDebitCached)
            return nDebitCached;
        nDebitCached = pwallet->GetDebit(*this);
        fDebitCached = true;
        return nDebitCached;
    }

    int64 GetCredit(bool fUseCache=true) const
    {
        // Must wait until coinbase is safely deep enough in the chain before valuing it
        if (IsCoinBase() && GetBlocksToMaturity() > 0)
            return 0;

        // GetBalance can assume transactions in mapWallet won't change
        if (fUseCache && fCreditCached)
            return nCreditCached;
        nCreditCached = pwallet->GetCredit(*this);
        fCreditCached = true;
        return nCreditCached;
    }

    int64 GetAvailableCredit(bool fUseCache=true) const
    {
        // Must wait until coinbase is safely deep enough in the chain before valuing it
        if (IsCoinBase() && GetBlocksToMaturity() > 0)
            return 0;

        if (fUseCache && fAvailableCreditCached)
            return nAvailableCreditCached;

        int64 nCredit = 0;
        for (int i = 0; i < vout.size(); i++)
        {
            if (!IsSpent(i))
            {
                const CTxOut &txout = vout[i];
                nCredit += pwallet->GetCredit(txout);
                if (!MoneyRange(nCredit))
                    throw std::runtime_error("CWalletTx::GetAvailableCredit() : value out of range");
            }
        }

        nAvailableCreditCached = nCredit;
        fAvailableCreditCached = true;
        return nCredit;
    }


    int64 GetChange() const
    {
        if (fChangeCached)
            return nChangeCached;
        nChangeCached = pwallet->GetChange(*this);
        fChangeCached = true;
        return nChangeCached;
    }

    void GetAmounts(int64& nGeneratedImmature, int64& nGeneratedMature, std::list<std::pair<CBitcoinAddress, int64> >& listReceived,
                    std::list<std::pair<CBitcoinAddress, int64> >& listSent, int64& nFee, std::string& strSentAccount) const;

    void GetAccountAmounts(const std::string& strAccount, int64& nGenerated, int64& nReceived, 
                           int64& nSent, int64& nFee) const;

    bool IsFromMe() const
    {
        return (GetDebit() > 0);
    }

    bool IsConfirmed() const
    {
        // Quick answer in most cases
        if (!IsFinal())
            return false;
        if (GetDepthInMainChain() >= 1)
            return true;
        if (!IsFromMe()) // using wtx's cached debit
            return false;

        // If no confirmations but it's from us, we can still
        // consider it confirmed if all dependencies are confirmed
        std::map<uint256, const CMerkleTx*> mapPrev;
        std::vector<const CMerkleTx*> vWorkQueue;
        vWorkQueue.reserve(vtxPrev.size()+1);
        vWorkQueue.push_back(this);
        for (int i = 0; i < vWorkQueue.size(); i++)
        {
            const CMerkleTx* ptx = vWorkQueue[i];

            if (!ptx->IsFinal())
                return false;
            if (ptx->GetDepthInMainChain() >= 1)
                continue;
            if (!pwallet->IsFromMe(*ptx))
                return false;

            if (mapPrev.empty())
                BOOST_FOREACH(const CMerkleTx& tx, vtxPrev)
                    mapPrev[tx.GetHash()] = &tx;

            BOOST_FOREACH(const CTxIn& txin, ptx->vin)
            {
                if (!mapPrev.count(txin.prevout.hash))
                    return false;
                vWorkQueue.push_back(mapPrev[txin.prevout.hash]);
            }
        }
        return true;
    }

    bool WriteToDisk();

    int64 GetTxTime() const;
    int GetRequestCount() const;

    void AddSupportingTransactions(CTxDB& txdb);

    bool AcceptWalletTransaction(CTxDB& txdb, bool fCheckInputs=true);
    bool AcceptWalletTransaction();

    void RelayWalletTransaction(CTxDB& txdb);
    void RelayWalletTransaction();
};


//
// Private key that includes an expiration date in case it never gets used.
//
class CWalletKey
{
public:
    CPrivKey vchPrivKey;
    int64 nTimeCreated;
    int64 nTimeExpires;
    std::string strComment;
    //// todo: add something to note what created it (user, getnewaddress, change)
    ////   maybe should have a map<string, string> property map

    CWalletKey(int64 nExpires=0)
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






//
// Account information.
// Stored in wallet with key "acc"+string account name
//
class CAccount
{
public:
    std::vector<unsigned char> vchPubKey;

    CAccount()
    {
        SetNull();
    }

    void SetNull()
    {
        vchPubKey.clear();
    }

    IMPLEMENT_SERIALIZE
    (
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(vchPubKey);
    )
};



//
// Internal transfers.
// Database key is acentry<account><counter>
//
class CAccountingEntry
{
public:
    std::string strAccount;
    int64 nCreditDebit;
    int64 nTime;
    std::string strOtherAccount;
    std::string strComment;

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
    }

    IMPLEMENT_SERIALIZE
    (
        if (!(nType & SER_GETHASH))
            READWRITE(nVersion);
        // Note: strAccount is serialized as part of the key, not here.
        READWRITE(nCreditDebit);
        READWRITE(nTime);
        READWRITE(strOtherAccount);
        READWRITE(strComment);
    )
};

bool GetWalletFile(CWallet* pwallet, std::string &strWalletFileOut);

#endif
