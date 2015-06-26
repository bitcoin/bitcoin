// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "corewallet/corewallet_wallet.h"

#include "base58.h"
#include "eccryptoverify.h"
#include "random.h"
#include "timedata.h"
#include "util.h"
#include "utilstrencodings.h"

#include <stdint.h>

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

namespace CoreWallet {

bool bSpendZeroConfChange = true;

bool Wallet::LoadKey(const CKey& key, const CPubKey &pubkey)
{
    return CCryptoKeyStore::AddKeyPubKey(key, pubkey);
}

bool Wallet::AddKeyPubKey(const CKey& secret, const CPubKey &pubkey)
{
    if (!CCryptoKeyStore::AddKeyPubKey(secret, pubkey))
        return false;
    
    return walletCacheDB->WriteKey(pubkey, secret.GetPrivKey(), mapKeyMetadata[pubkey.GetID()]);
}

bool Wallet::LoadKeyMetadata(const CPubKey &pubkey, const CKeyMetadata &meta)
{
    LOCK(cs_coreWallet);
    if (meta.nCreateTime && (!nTimeFirstKey || meta.nCreateTime < nTimeFirstKey))
        nTimeFirstKey = meta.nCreateTime;

    mapKeyMetadata[pubkey.GetID()] = meta;
    return true;
}

bool Wallet::SetAddressBook(const CTxDestination& address, const std::string& strPurpose)
{
    LOCK(cs_coreWallet);
    std::map<CTxDestination, CAddressBookMetadata> ::iterator mi = mapAddressBook.find(address);
    if(mi == mapAddressBook.end())
        mapAddressBook[address] = CAddressBookMetadata();
    
    mapAddressBook[address].purpose = strPurpose;
    
    return walletCacheDB->Write(std::make_pair(std::string("adrmeta"), CBitcoinAddress(address).ToString()), mapAddressBook[address]);
}



const unsigned int HD_MAX_DEPTH = 20;

bool Wallet::HDSetChainPath(const std::string& chainPathIn, bool generateMaster, CKeyingMaterial& vSeed, HDChainID& chainId, bool overwrite)
{
    LOCK(cs_coreWallet);

    if (IsLocked())
        return false;

    if (chainPathIn[0] != 'm')
        throw std::runtime_error("CoreWallet::SetHDChainPath(): Non masterkey chainpaths are not allowed.");

    if (chainPathIn.find_first_of("c", 1) == std::string::npos)
        throw std::runtime_error("CoreWallet::SetHDChainPath(): 'c' (internal/external chain selection) is requires in the given chainpath.");

    if (chainPathIn.find_first_not_of("0123456789'/mch", 0) != std::string::npos)
        throw std::runtime_error("CoreWallet::SetHDChainPath(): Invalid chainpath.");

    std::string newChainPath = chainPathIn;
    boost::to_lower(newChainPath);
    boost::erase_all(newChainPath, " ");
    boost::replace_all(newChainPath, "h", "'"); //support h instead of ' to allow easy JSON input over cmd line
    if (newChainPath.size() > 0 && *newChainPath.rbegin() == '/')
        newChainPath.resize(newChainPath.size() - 1);

    std::vector<std::string> pathFragments;
    boost::split(pathFragments, newChainPath, boost::is_any_of("/"));

    if (pathFragments.size() > HD_MAX_DEPTH)
        throw std::runtime_error("CoreWallet::SetHDChainPath(): Max chain depth ("+itostr(HD_MAX_DEPTH)+") exceeded!");

    int64_t nCreationTime = GetTime();
    CHDChain newChain(nCreationTime);
    newChain.chainPath = newChainPath;
    CExtKey parentKey;
    BOOST_FOREACH(std::string fragment, pathFragments)
    {
        bool harden = false;
        if (*fragment.rbegin() == '\'')
        {
            harden = true;
            fragment = fragment.substr(0,fragment.size()-1);
        }

        if (fragment == "m")
        {
            //generate a master key seed
            //currently seed size is fixed to 256bit
            assert(vSeed.size() == 32);
            if (generateMaster)
            {
                RandAddSeedPerfmon();
                do {
                    GetRandBytes(&vSeed[0], vSeed.size());
                } while (!eccrypto::Check(&vSeed[0]));
            }

            CExtKey bip32MasterKey;
            bip32MasterKey.SetMaster(&vSeed[0], vSeed.size());

            CBitcoinExtKey b58key;
            b58key.SetKey(bip32MasterKey);
            chainId = bip32MasterKey.key.GetPubKey().GetHash();

            //only one chain per master seed is allowed
            CHDChain possibleChain;
            if (GetChain(chainId, possibleChain) && possibleChain.IsValid())
                throw std::runtime_error("CoreWallet::SetHDChainPath(): Only one chain per masterseed is allowed.");

            if (!AddMasterSeed(chainId, vSeed))
                throw std::runtime_error("CoreWallet::SetHDChainPath(): Could not store master seed.");

            //keep the master pubkeyhash for chain identifying
            newChain.chainHash = chainId;

            if (IsCrypted())
            {
                std::vector<unsigned char> vchCryptedSecret;
                GetCryptedMasterSeed(chainId, vchCryptedSecret);

                if (!walletPrivateDB->WriteHDCryptedMasterSeed(chainId, vchCryptedSecret))
                    throw std::runtime_error("CoreWallet::SetHDChainPath(): Writing hdmasterseed failed!");
            }
            else
            {
                if (!walletPrivateDB->WriteHDMasterSeed(chainId, vSeed))
                    throw std::runtime_error("CoreWallet::SetHDChainPath(): Writing cryted hdmasterseed failed!");
            }

            //set active hd chain
            activeHDChain = chainId;
            if (!walletCacheDB->WriteHDAchiveChain(chainId))
                throw std::runtime_error("CoreWallet::SetHDChainPath(): Writing active hd chain failed!");

            parentKey = bip32MasterKey;
        }
        else if (fragment == "c")
        {
            harden = false; //external / internal chain root keys can not be hardened
            CExtPubKey parentExtPubKey = parentKey.Neuter();
            parentExtPubKey.Derive(newChain.externalPubKey, 0);
            parentExtPubKey.Derive(newChain.internalPubKey, 1);

            AddChain(newChain);

            if (!walletCacheDB->WriteHDChain(newChain))
                throw std::runtime_error("CoreWallet::SetHDChainPath(): Writing new chain failed!");
        }
        else
        {
            CExtKey childKey;
            int32_t nIndex;
            if (!ParseInt32(fragment,&nIndex))
                return false;
            parentKey.Derive(childKey, (harden ? 0x80000000 : 0)+nIndex);
            parentKey = childKey;
        }
    }

    return true;
}

bool Wallet::HDGetChildPubKeyAtIndex(const HDChainID& chainIDIn, CPubKey &pubKeyOut, std::string& newKeysChainpath, unsigned int nIndex, bool internal)
{
    AssertLockHeld(cs_coreWallet);

    //check possible null chainId and load current active chain
    CHDChain chain;
    HDChainID chainID = chainIDIn;
    if (chainID.IsNull())
        chainID = activeHDChain;

    if (!GetChain(chainID, chain) || !chain.IsValid())
        throw std::runtime_error("CoreWallet::HDGetChildPubKeyAtIndex(): Selected chain is not vailid!");

    CHDPubKey newHdPubKey;
    if (!DeriveHDPubKeyAtIndex(chainID, newHdPubKey, nIndex, internal))
        throw std::runtime_error("CoreWallet::HDGetChildPubKeyAtIndex(): Deriving child key faild!");

    // Create new metadata
    int64_t nCreationTime = GetTime();
    mapKeyMetadata[newHdPubKey.pubkey.GetID()] = CKeyMetadata(nCreationTime);
    if (!nTimeFirstKey || nCreationTime < nTimeFirstKey)
        nTimeFirstKey = nCreationTime;

    if (!LoadHDPubKey(newHdPubKey))
        throw std::runtime_error("CoreWallet::HDGetChildPubKeyAtIndex(): Add key to keystore failed!");

    if (!walletCacheDB->WriteHDPubKey(newHdPubKey, mapKeyMetadata[newHdPubKey.pubkey.GetID()]))
        throw std::runtime_error("CoreWallet::HDGetChildPubKeyAtIndex(): Writing pubkey failed!");

    pubKeyOut = newHdPubKey.pubkey;

    newKeysChainpath = chain.chainPath;
    boost::replace_all(newKeysChainpath, "c", itostr(internal)); //replace the chain switch index
    newKeysChainpath += "/"+itostr(nIndex);

    return true;
}

bool Wallet::HDGetNextChildPubKey(const HDChainID& chainIDIn, CPubKey &pubKeyOut, std::string& newKeysChainpath, bool internal)
{
    AssertLockHeld(cs_coreWallet);

    CHDChain chain;
    HDChainID chainID = chainIDIn;
    if (chainID.IsNull())
        chainID = activeHDChain;

    if (!GetChain(chainID, chain) || !chain.IsValid())
        throw std::runtime_error("CoreWallet::HDGetNextChildPubKey(): Selected chain is not vailid!");

    unsigned int nextIndex = GetNextChildIndex(chainID, internal);
    if (!HDGetChildPubKeyAtIndex(chainID, pubKeyOut, newKeysChainpath, nextIndex, internal))
        return false;

    return true;
}

bool Wallet::EncryptHDSeeds(CKeyingMaterial& vMasterKeyIn)
{
    EncryptSeeds();

    std::vector<HDChainID> chainIds;
    GetAvailableChainIDs(chainIds);

    BOOST_FOREACH(HDChainID& chainId, chainIds)
    {
        std::vector<unsigned char> vchCryptedSecret;
        if (!GetCryptedMasterSeed(chainId, vchCryptedSecret))
            throw std::runtime_error("CoreWallet::EncryptHDSeeds(): Encrypting seeds failed!");
        
        if (!walletPrivateDB->WriteHDCryptedMasterSeed(chainId, vchCryptedSecret))
            throw std::runtime_error("CoreWallet::EncryptHDSeeds(): Writing hdmasterseed failed!");
    }
    
    return true;
}

bool Wallet::HDSetActiveChainID(const HDChainID& chainID, bool check)
{
    LOCK(cs_coreWallet);
    CHDChain chainOut;
    
    //do the chainID check optional because we need a way of setting the chainID before the acctual chain is loaded into the memory
    //this is becuase of the way we load the chains from the wallet.dat
    if (check && !GetChain(chainID, chainOut)) //TODO: implement FindChain() to avoild copy of CHDChain
        return false;
    
    activeHDChain = chainID;
    return true;
}

bool Wallet::HDGetActiveChainID(HDChainID& chainID)
{
    LOCK(cs_coreWallet);
    chainID = activeHDChain;
    return true;
}

/*
 ######################
 # WTX list stack     #
 ######################
 */

const WalletTx* Wallet::GetWalletTx(const uint256& hash) const
{
    LOCK(cs_coreWallet);
    std::map<uint256, WalletTx>::const_iterator it = mapWallet.find(hash);
    if (it == mapWallet.end())
        return NULL;
    return &(it->second);
}

Wallet::TxItems Wallet::OrderedTxItems()
{
    AssertLockHeld(cs_coreWallet); // mapWallet

    // First: get all WalletTx and CAccountingEntry into a sorted-by-order multimap.
    TxItems txOrdered;

    // Note: maintaining indices in the database of (account,time) --> txid and (account, time) --> acentry
    // would make this much faster for applications that do this a lot.
    for (std::map<uint256, WalletTx>::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
    {
        WalletTx* wtx = &((*it).second);
        txOrdered.insert(std::make_pair(wtx->nOrderPos, TxPair(wtx, "")));
    }
    
    return txOrdered;
}

/*
 ########################
 # Coin Lock Stack #
 ########################
 */
void Wallet::LockCoin(COutPoint& output)
{
    AssertLockHeld(cs_coreWallet); // setLockedCoins
    setLockedCoins.insert(output);
}

void Wallet::UnlockCoin(COutPoint& output)
{
    AssertLockHeld(cs_coreWallet); // setLockedCoins
    setLockedCoins.erase(output);
}

void Wallet::UnlockAllCoins()
{
    AssertLockHeld(cs_coreWallet); // setLockedCoins
    setLockedCoins.clear();
}

bool Wallet::IsLockedCoin(uint256 hash, unsigned int n) const
{
    AssertLockHeld(cs_coreWallet); // setLockedCoins
    COutPoint outpt(hash, n);

    return (setLockedCoins.count(outpt) > 0);
}

void Wallet::ListLockedCoins(std::vector<COutPoint>& vOutpts)
{
    AssertLockHeld(cs_coreWallet); // setLockedCoins
    for (std::set<COutPoint>::iterator it = setLockedCoins.begin();
         it != setLockedCoins.end(); it++) {
        COutPoint outpt = (*it);
        vOutpts.push_back(outpt);
    }
}

/*
 ########################
 # Coin Selection Stack #
 ########################
 */

/**
 * populate vCoins with vector of available COutputs.
 */

bool Wallet::IsTrustedWTx(const WalletTx &wtx) const
{
    AssertLockHeld(cs_coreWallet);

    // Quick answer in most cases
    if (!coreInterface.CheckFinalTx(wtx))
        return false;
    int nDepth = wtx.GetDepthInMainChain();
    if (nDepth >= 1)
        return true;
    if (nDepth < 0)
        return false;
    if (!bSpendZeroConfChange || !wtx.IsFromMe(ISMINE_ALL)) // using wtx's cached debit
        return false;

    // Trusted if all inputs are from us and are in the mempool:
    BOOST_FOREACH(const CTxIn& txin, wtx.vin)
    {
        // Transactions not sent by us: not trusted
        const WalletTx* parent = GetWalletTx(txin.prevout.hash);
        if (parent == NULL)
            return false;
        const CTxOut& parentOut = parent->vout[txin.prevout.n];
        if (IsMine(parentOut) != ISMINE_SPENDABLE)
            return false;
    }
    return true;
}

void Wallet::AvailableCoins(std::vector<COutput>& vCoins, bool fOnlyConfirmed, const CCoinControl *coinControl, bool fIncludeZeroValue) const
{
    vCoins.clear();

    {
        LOCK(cs_coreWallet);
        for (std::map<uint256, WalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const uint256& wtxid = it->first;
            const WalletTx* pcoin = &(*it).second;

            if (fOnlyConfirmed && !IsTrustedWTx(*pcoin))
                continue;

            if (pcoin->IsCoinBase() && pcoin->GetBlocksToMaturity() > 0)
                continue;

            int nDepth = pcoin->GetDepthInMainChain();
            if (nDepth < 0)
                continue;

            for (unsigned int i = 0; i < pcoin->vout.size(); i++) {
                isminetype mine = IsMine(pcoin->vout[i]);
                if (!(IsSpent(wtxid, i)) && mine != ISMINE_NO &&
                    !IsLockedCoin((*it).first, i) && (pcoin->vout[i].nValue > 0 || fIncludeZeroValue) &&
                    (!coinControl || !coinControl->HasSelected() || coinControl->fAllowOtherInputs || coinControl->IsSelected((*it).first, i)))
                    vCoins.push_back(COutput(pcoin, i, nDepth, (mine & ISMINE_SPENDABLE) != ISMINE_NO));
            }
        }
    }
}

bool Wallet::RelayWalletTransaction(const WalletTx &wtx)
{
    assert(GetBroadcastTransactions());
    if (!wtx.IsCoinBase())
    {
        if (wtx.GetDepthInMainChain() == 0) {
            LogPrintf("Relaying wtx %s\n", wtx.GetHash().ToString());
            coreInterface.RelayTransaction((CTransaction)wtx);
            return true;
        }
    }
    return false;
}

/*
 ######################
 # Receiving tx stack #
 ######################
*/

std::set<uint256> Wallet::GetConflicts(const uint256& txid) const
{
    std::set<uint256> result;
    AssertLockHeld(cs_coreWallet);

    std::map<uint256, WalletTx>::const_iterator it = mapWallet.find(txid);
    if (it == mapWallet.end())
        return result;
    const WalletTx& wtx = it->second;

    std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range;

    BOOST_FOREACH(const CTxIn& txin, wtx.vin)
    {
        if (mapTxSpends.count(txin.prevout) <= 1)
            continue;  // No conflict if zero or one spends
        range = mapTxSpends.equal_range(txin.prevout);
        for (TxSpends::const_iterator it = range.first; it != range.second; ++it)
            result.insert(it->second);
    }
    return result;
}

/**
 * Outpoint is spent if any non-conflicted transaction
 * spends it:
 */
bool Wallet::IsSpent(const uint256& hash, unsigned int n) const
{
    AssertLockHeld(cs_coreWallet);

    const COutPoint outpoint(hash, n);
    std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range;
    range = mapTxSpends.equal_range(outpoint);

    for (TxSpends::const_iterator it = range.first; it != range.second; ++it)
    {
        const uint256& wtxid = it->second;
        std::map<uint256, WalletTx>::const_iterator mit = mapWallet.find(wtxid);
        if (mit != mapWallet.end() && mit->second.GetDepthInMainChain() >= 0)
            return true; // Spent
    }
    return false;
}

void Wallet::AddToSpends(const COutPoint& outpoint, const uint256& wtxid)
{
    AssertLockHeld(cs_coreWallet);

    mapTxSpends.insert(std::make_pair(outpoint, wtxid));

    std::pair<TxSpends::iterator, TxSpends::iterator> range;
    range = mapTxSpends.equal_range(outpoint);
    SyncMetaData(range);
}


void Wallet::AddToSpends(const uint256& wtxid)
{
    AssertLockHeld(cs_coreWallet);

    assert(mapWallet.count(wtxid));
    WalletTx& thisTx = mapWallet[wtxid];
    if (thisTx.IsCoinBase()) // Coinbases don't spend anything!
        return;
    
    BOOST_FOREACH(const CTxIn& txin, thisTx.vin)
    AddToSpends(txin.prevout, wtxid);
}

bool Wallet::AddToWallet(const WalletTx& wtxIn, bool fFromLoadWallet)
{
    LOCK(cs_coreWallet);

    uint256 hash = wtxIn.GetHash();

    if (fFromLoadWallet)
    {
        //TODO add CheckTransaction from main.cpp like check
        mapWallet[hash] = wtxIn;
        mapWallet[hash].BindWallet(this);
        AddToSpends(hash);
    }
    else
    {
        // Inserts only if not already there, returns tx inserted or tx found
        std::pair<std::map<uint256, WalletTx>::iterator, bool> ret = mapWallet.insert(std::make_pair(hash, wtxIn));
        WalletTx& wtx = (*ret.first).second;
        wtx.BindWallet(this);
        bool fInsertedNew = ret.second;
        if (fInsertedNew)
        {
            wtx.nTimeReceived = GetAdjustedTime();
            wtx.nOrderPos = 0; //IncOrderPosNext(pwalletdb);

            wtx.nTimeSmart = wtx.nTimeReceived;
            if (!wtxIn.hashBlock.IsNull())
            {
                const CBlockIndex *possibleIndex = GetBlockIndex(wtxIn.hashBlock);
                if (possibleIndex)
                {
                    int64_t latestNow = wtx.nTimeReceived;
                    int64_t latestEntry = 0;
                    {
                        // Tolerate times up to the last timestamp in the wallet not more than 5 minutes into the future
                        int64_t latestTolerated = latestNow + 300;
                        TxItems txOrdered = OrderedTxItems();
                        for (TxItems::reverse_iterator it = txOrdered.rbegin(); it != txOrdered.rend(); ++it)
                        {
                            WalletTx *const pwtx = (*it).second.first;
                            if (pwtx == &wtx)
                                continue;

                            int64_t nSmartTime = 0;
                            if (pwtx)
                            {
                                nSmartTime = pwtx->nTimeSmart;
                                if (!nSmartTime)
                                    nSmartTime = pwtx->nTimeReceived;
                            }
                            if (nSmartTime <= latestTolerated)
                            {
                                latestEntry = nSmartTime;
                                if (nSmartTime > latestNow)
                                    latestNow = nSmartTime;
                                break;
                            }
                        }
                    }

                    int64_t blocktime = possibleIndex->GetBlockTime();
                    wtx.nTimeSmart = std::max(latestEntry, std::min(blocktime, latestNow));
                }
                else
                    LogPrintf("AddToWallet(): found %s in block %s not in index\n",
                              wtxIn.GetHash().ToString(),
                              wtxIn.hashBlock.ToString());
            }
            AddToSpends(hash);
        }

        bool fUpdated = false;
        if (!fInsertedNew)
        {
            // Merge
            if (!wtxIn.hashBlock.IsNull() && wtxIn.hashBlock != wtx.hashBlock)
            {
                wtx.hashBlock = wtxIn.hashBlock;
                fUpdated = true;
            }
            if (wtxIn.nIndex != -1 && (wtxIn.vMerkleBranch != wtx.vMerkleBranch || wtxIn.nIndex != wtx.nIndex))
            {
                wtx.vMerkleBranch = wtxIn.vMerkleBranch;
                wtx.nIndex = wtxIn.nIndex;
                fUpdated = true;
            }
            if (wtxIn.fFromMe && wtxIn.fFromMe != wtx.fFromMe)
            {
                wtx.fFromMe = wtxIn.fFromMe;
                fUpdated = true;
            }
        }

        //// debug print
        LogPrintf("AddToWallet %s  %s%s\n", wtxIn.GetHash().ToString(), (fInsertedNew ? "new" : ""), (fUpdated ? "update" : ""));

        // Write to disk
        if (fInsertedNew || fUpdated)
            if (!WriteWTXToDisk(wtx))
                return false;

        // Break debit/credit balance caches:
        wtx.MarkDirty();

        // Notify UI of new or updated transaction
        NotifyTransactionChanged(this, hash, fInsertedNew ? CT_NEW : CT_UPDATED);
    }
    return true;
}

/**
 * Add a transaction to the wallet, or update it.
 * pblock is optional, but should be provided if the transaction is known to be in a block.
 * If fUpdate is true, existing transactions will be updated.
 */
bool Wallet::AddToWalletIfInvolvingMe(const CTransaction& tx, const CBlock* pblock, bool fUpdate)
{
    {
        AssertLockHeld(cs_coreWallet);
        bool fExisted = mapWallet.count(tx.GetHash()) != 0;
        if (fExisted && !fUpdate) return false;
        if (fExisted || IsMine(tx) || IsFromMe(tx))
        {
            WalletTx wtx(this,tx);

            // Get merkle branch if transaction was found in a block
            if (pblock)
                wtx.SetMerkleBranch(*pblock);

            return AddToWallet(wtx, false);
        }
    }
    return false;
}

isminetype Wallet::IsMine(const CTxIn &txin) const
{
    {
        LOCK(cs_coreWallet);
        std::map<uint256, WalletTx>::const_iterator mi = mapWallet.find(txin.prevout.hash);
        if (mi != mapWallet.end())
        {
            const WalletTx& prev = (*mi).second;
            if (txin.prevout.n < prev.vout.size())
                return IsMine(prev.vout[txin.prevout.n]);
        }
    }
    return ISMINE_NO;
}

CAmount Wallet::GetDebit(const CTxIn &txin, const isminefilter& filter) const
{
    {
        LOCK(cs_coreWallet);
        std::map<uint256, WalletTx>::const_iterator mi = mapWallet.find(txin.prevout.hash);
        if (mi != mapWallet.end())
        {
            const WalletTx& prev = (*mi).second;
            if (txin.prevout.n < prev.vout.size())
                if (IsMine(prev.vout[txin.prevout.n]) & filter)
                    return prev.vout[txin.prevout.n].nValue;
        }
    }
    return 0;
}

isminetype Wallet::IsMine(const CTxOut& txout) const
{
    return CoreWallet::IsMine(*this, txout.scriptPubKey);
}

CAmount Wallet::GetCredit(const CTxOut& txout, const isminefilter& filter) const
{
    if (!MoneyRange(txout.nValue))
        throw std::runtime_error("Wallet::GetCredit(): value out of range");
    return ((IsMine(txout) & filter) ? txout.nValue : 0);
}

bool Wallet::IsMine(const CTransaction& tx) const
{
    BOOST_FOREACH(const CTxOut& txout, tx.vout)
    if (IsMine(txout))
        return true;
    return false;
}

bool Wallet::IsFromMe(const CTransaction& tx) const
{
    return (GetDebit(tx, ISMINE_ALL) > 0);
}

bool Wallet::WriteWTXToDisk(const WalletTx &wtx)
{
    return walletCacheDB->WriteTx(wtx.GetHash(), wtx);
}

void Wallet::SyncMetaData(std::pair<TxSpends::iterator, TxSpends::iterator> range)
{
    // We want all the wallet transactions in range to have the same metadata as
    // the oldest (smallest nOrderPos).
    // So: find smallest nOrderPos:

    int nMinOrderPos = std::numeric_limits<int>::max();
    const WalletTx* copyFrom = NULL;
    for (TxSpends::iterator it = range.first; it != range.second; ++it)
    {
        const uint256& hash = it->second;
        int n = mapWallet[hash].nOrderPos;
        if (n < nMinOrderPos)
        {
            nMinOrderPos = n;
            copyFrom = &mapWallet[hash];
        }
    }
    // Now copy data from copyFrom to rest:
    for (TxSpends::iterator it = range.first; it != range.second; ++it)
    {
        const uint256& hash = it->second;
        WalletTx* copyTo = &mapWallet[hash];
        if (copyFrom == copyTo) continue;
        copyTo->mapValue = copyFrom->mapValue;
        copyTo->vOrderForm = copyFrom->vOrderForm;
        // fTimeReceivedIsTxTime not copied on purpose
        // nTimeReceived not copied on purpose
        copyTo->nTimeSmart = copyFrom->nTimeSmart;
        copyTo->fFromMe = copyFrom->fFromMe;
        // nOrderPos not copied on purpose
        // cached members not copied on purpose
    }
}

void Wallet::SyncTransaction(const CTransaction& tx, const CBlock* pblock)
{
    LOCK(cs_coreWallet);
    if (!AddToWalletIfInvolvingMe(tx, pblock, true))
        return; // Not one of ours

    // If a transaction changes 'conflicted' state, that changes the balance
    // available of the outputs it spends. So force those to be
    // recomputed, also:
    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    {
        if (mapWallet.count(txin.prevout.hash))
            mapWallet[txin.prevout.hash].MarkDirty();
    }
}

/*
 #############################
 # Balance calculation stack #
 #############################
 */

CAmount Wallet::GetDebit(const CTransaction& tx, const isminefilter& filter) const
{
    CAmount nDebit = 0;
    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    {
        nDebit += GetDebit(txin, filter);
        if (!MoneyRange(nDebit))
            throw std::runtime_error("Wallet::GetDebit(): value out of range");
    }
    return nDebit;
}

CAmount Wallet::GetCredit(const CTransaction& tx, const isminefilter& filter) const
{
    CAmount nCredit = 0;
    BOOST_FOREACH(const CTxOut& txout, tx.vout)
    {
        nCredit += GetCredit(txout, filter);
        if (!MoneyRange(nCredit))
            throw std::runtime_error("Wallet::GetCredit(): value out of range");
    }
    return nCredit;
}

CAmount Wallet::GetBalance(const enum CREDIT_DEBIT_TYPE &balanceType, const isminefilter& filter) const
{
    CAmount nTotal = 0;
    {
        LOCK(cs_coreWallet);
        for (std::map<uint256, WalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const WalletTx* pcoin = &(*it).second;
            if (
                ( balanceType == CREDIT_DEBIT_TYPE_AVAILABLE && IsTrustedWTx(*pcoin) )
                || //OR
                ( balanceType == CREDIT_DEBIT_TYPE_UNCONFIRMED && !IsTrustedWTx(*pcoin) && pcoin->GetDepthInMainChain() == 0)
                || //OR
                ( balanceType == CREDIT_DEBIT_TYPE_IMMATURE && pcoin->IsCoinBase() && pcoin->GetBlocksToMaturity() > 0 && pcoin->IsInMainChain())
                )
            {
                CAmount cacheOut;
                bool cacheFound = pcoin->GetCache(balanceType, filter, cacheOut);

                if (balanceType == CREDIT_DEBIT_TYPE_IMMATURE && pcoin->IsCoinBase() && pcoin->GetBlocksToMaturity() > 0 && pcoin->IsInMainChain())
                {
                    //shortcut for imature balance
                    if (cacheFound)
                    {
                        nTotal += cacheOut;
                        break;
                    }

                    CAmount nCredit = GetCredit(*pcoin, filter);
                    pcoin->SetCache(balanceType, filter, nCredit);
                    nTotal += nCredit;
                    break;
                }

                // Must wait until coinbase is safely deep enough in the chain before valuing it
                if (pcoin->IsCoinBase() && pcoin->GetBlocksToMaturity() < 0)
                    break;

                if (cacheFound)
                {
                    nTotal += cacheOut;
                    break;
                }

                CAmount nCredit = 0;
                uint256 hashTx = pcoin->GetHash();
                for (unsigned int i = 0; i < pcoin->vout.size(); i++)
                {
                    if (!IsSpent(hashTx, i))
                    {
                        const CTxOut &txout = pcoin->vout[i];
                        nCredit += GetCredit(txout, filter);
                    }
                }
                pcoin->SetCache(balanceType, filter, nCredit);
                nTotal += nCredit;
            }
        }
    }
    return nTotal;
}

/*
 #########################
 # Full Node Interaction #
 #########################
 */

const CBlockIndex* Wallet::GetBlockIndex(uint256 blockhash, bool inActiveChain) const
{
    LOCK(cs_coreWallet);
    return coreInterface.GetBlockIndex(blockhash, inActiveChain);
}

int Wallet::GetActiveChainHeight() const
{
    LOCK(cs_coreWallet);
    return coreInterface.GetActiveChainHeight();
}

int Wallet::MempoolExists(uint256 hash) const
{
    LOCK(cs_coreWallet);
    return coreInterface.MempoolExists(hash);
}

bool Wallet::AcceptToMemoryPool(const WalletTx &wtx, bool fLimitFree, bool fRejectAbsurdFee)
{
    LOCK(cs_coreWallet);
    CValidationState state;
    return coreInterface.AcceptToMemoryPool(state, wtx, fLimitFree, fRejectAbsurdFee);
}

}; //end namespace