// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "corewallet/corewallet_wallet.h"

#include "consensus/consensus.h"

#include "script/script.h"
#include "script/sign.h"

#include "base58.h"
#include "eccryptoverify.h"
#include "random.h"
#include "timedata.h"
#include "util.h"
#include "utilmoneystr.h"
#include "utilstrencodings.h"

#include <stdint.h>

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

namespace CoreWallet {

bool bSpendZeroConfChange = true;
bool fSendFreeTransactions = false;
bool fPayAtLeastCustomFee = true;
unsigned int nTxConfirmTarget = 2;
CFeeRate payTxFee(0);
CAmount maxTxFee = 0.1 * COIN;
CFeeRate Wallet::minTxFee = CFeeRate(1000);


Wallet::Wallet(std::string strWalletFileIn)
{
    //instantiate a wallet backend object and maps the stored values
    walletPrivateDB = new FileDB(strWalletFileIn+".private.logdb");
    walletPrivateDB->LoadWallet(this);

    walletCacheDB = new FileDB(strWalletFileIn+".cache.logdb");
    walletCacheDB->LoadWallet(this);

    this->SetBroadcastTransactions(GetBoolArg("-walletbroadcast", true));

    coreInterface = CoreInterface();
}

bool Wallet::InMemAddKey(const CKey& key, const CPubKey &pubkey)
{
    return CCryptoKeyStore::AddKeyPubKey(key, pubkey);
}

bool Wallet::AddAndStoreKeyPubKey(const CKey& secret, const CPubKey &pubkey)
{
    if (!CCryptoKeyStore::AddKeyPubKey(secret, pubkey))
        return false;
    
    return walletCacheDB->WriteKey(pubkey, secret.GetPrivKey(), mapKeyMetadata[pubkey.GetID()]);
}

bool Wallet::InMemAddKeyMetadata(const CPubKey &pubkey, const CKeyMetadata &meta)
{
    LOCK(cs_coreWallet);
    if (meta.nCreateTime && (!nTimeFirstKey || meta.nCreateTime < nTimeFirstKey))
        nTimeFirstKey = meta.nCreateTime;

    mapKeyMetadata[pubkey.GetID()] = meta;
    return true;
}

bool Wallet::SetAndStoreAddressBook(const CTxDestination& address, const std::string& strPurpose)
{
    LOCK(cs_coreWallet);
    std::map<CTxDestination, CAddressBookMetadata> ::iterator mi = mapAddressBook.find(address);
    if(mi == mapAddressBook.end())
        mapAddressBook[address] = CAddressBookMetadata();
    
    mapAddressBook[address].purpose = strPurpose;

    //TODO: move to database and not directly call ->Write()
    return walletCacheDB->Write(std::make_pair(std::string(kvs_address_book_metadata_key), CBitcoinAddress(address).ToString()), mapAddressBook[address]);
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

Wallet::WtxItems Wallet::OrderedTxItems()
{
    AssertLockHeld(cs_coreWallet); // mapWallet

    // First: get all WalletTx and CAccountingEntry into a sorted-by-order multimap.
    WtxItems txOrdered;

    // Note: maintaining indices in the database of (account,time) --> txid and (account, time) --> acentry
    // would make this much faster for applications that do this a lot.
    for (std::map<uint256, WalletTx>::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
    {
        WalletTx* wtx = &((*it).second);
        txOrdered.insert(std::make_pair(wtx->nOrderPos, wtx));
    }
    
    return txOrdered;
}

/*
 ###################
 # Coin Lock Stack #
 ###################
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
    if (!bSpendZeroConfChange || !IsFromMe(wtx, ISMINE_ALL)) // using wtx's cached debit
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


struct CompareValueOnly
{
    bool operator()(const std::pair<CAmount, std::pair<const WalletTx*, unsigned int> >& t1,
                    const std::pair<CAmount, std::pair<const WalletTx*, unsigned int> >& t2) const
    {
        return t1.first < t2.first;
    }
};

static void ApproximateBestSubset(std::vector<std::pair<CAmount, std::pair<const WalletTx*,unsigned int> > >vValue, const CAmount& nTotalLower, const CAmount& nTargetValue, std::vector<char>& vfBest, CAmount& nBest, int iterations = 1000)
{
    std::vector<char> vfIncluded;

    vfBest.assign(vValue.size(), true);
    nBest = nTotalLower;

    seed_insecure_rand();

    for (int nRep = 0; nRep < iterations && nBest != nTargetValue; nRep++)
    {
        vfIncluded.assign(vValue.size(), false);
        CAmount nTotal = 0;
        bool fReachedTarget = false;
        for (int nPass = 0; nPass < 2 && !fReachedTarget; nPass++)
        {
            for (unsigned int i = 0; i < vValue.size(); i++)
            {
                //The solver here uses a randomized algorithm,
                //the randomness serves no real security purpose but is just
                //needed to prevent degenerate behavior and it is important
                //that the rng is fast. We do not use a constant random sequence,
                //because there may be some privacy improvement by making
                //the selection random.
                if (nPass == 0 ? insecure_rand()&1 : !vfIncluded[i])
                {
                    nTotal += vValue[i].first;
                    vfIncluded[i] = true;
                    if (nTotal >= nTargetValue)
                    {
                        fReachedTarget = true;
                        if (nTotal < nBest)
                        {
                            nBest = nTotal;
                            vfBest = vfIncluded;
                        }
                        nTotal -= vValue[i].first;
                        vfIncluded[i] = false;
                    }
                }
            }
        }
    }
}

bool Wallet::SelectCoinsMinConf(const CAmount& nTargetValue, int nConfMine, int nConfTheirs, std::vector<COutput> vCoins,
                                 std::set<std::pair<const WalletTx*,unsigned int> >& setCoinsRet, CAmount& nValueRet) const
{
    setCoinsRet.clear();
    nValueRet = 0;

    // List of values less than target
    std::pair<CAmount, std::pair<const WalletTx*,unsigned int> > coinLowestLarger;
    coinLowestLarger.first = std::numeric_limits<CAmount>::max();
    coinLowestLarger.second.first = NULL;
    std::vector<std::pair<CAmount, std::pair<const WalletTx*,unsigned int> > > vValue;
    CAmount nTotalLower = 0;

    random_shuffle(vCoins.begin(), vCoins.end(), GetRandInt);

    BOOST_FOREACH(const COutput &output, vCoins)
    {
        if (!output.fSpendable)
            continue;

        const WalletTx *pcoin = output.tx;

        if (output.nDepth < (IsFromMe(*pcoin, ISMINE_ALL) ? nConfMine : nConfTheirs))
            continue;

        int i = output.i;
        CAmount n = pcoin->vout[i].nValue;

        std::pair<CAmount,std::pair<const WalletTx*,unsigned int> > coin = std::make_pair(n,std::make_pair(pcoin, i));

        if (n == nTargetValue)
        {
            setCoinsRet.insert(coin.second);
            nValueRet += coin.first;
            return true;
        }
        else if (n < nTargetValue + CENT)
        {
            vValue.push_back(coin);
            nTotalLower += n;
        }
        else if (n < coinLowestLarger.first)
        {
            coinLowestLarger = coin;
        }
    }

    if (nTotalLower == nTargetValue)
    {
        for (unsigned int i = 0; i < vValue.size(); ++i)
        {
            setCoinsRet.insert(vValue[i].second);
            nValueRet += vValue[i].first;
        }
        return true;
    }

    if (nTotalLower < nTargetValue)
    {
        if (coinLowestLarger.second.first == NULL)
            return false;
        setCoinsRet.insert(coinLowestLarger.second);
        nValueRet += coinLowestLarger.first;
        return true;
    }

    // Solve subset sum by stochastic approximation
    sort(vValue.rbegin(), vValue.rend(), CompareValueOnly());
    std::vector<char> vfBest;
    CAmount nBest;

    ApproximateBestSubset(vValue, nTotalLower, nTargetValue, vfBest, nBest, 1000);
    if (nBest != nTargetValue && nTotalLower >= nTargetValue + CENT)
        ApproximateBestSubset(vValue, nTotalLower, nTargetValue + CENT, vfBest, nBest, 1000);

    // If we have a bigger coin and (either the stochastic approximation didn't find a good solution,
    //                                   or the next bigger coin is closer), return the bigger coin
    if (coinLowestLarger.second.first &&
        ((nBest != nTargetValue && nBest < nTargetValue + CENT) || coinLowestLarger.first <= nBest))
    {
        setCoinsRet.insert(coinLowestLarger.second);
        nValueRet += coinLowestLarger.first;
    }
    else {
        for (unsigned int i = 0; i < vValue.size(); i++)
            if (vfBest[i])
            {
                setCoinsRet.insert(vValue[i].second);
                nValueRet += vValue[i].first;
            }

        LogPrint("selectcoins", "SelectCoins() best subset: ");
        for (unsigned int i = 0; i < vValue.size(); i++)
            if (vfBest[i])
                LogPrint("selectcoins", "%s ", FormatMoney(vValue[i].first));
        LogPrint("selectcoins", "total %s\n", FormatMoney(nBest));
    }

    return true;
}

bool Wallet::SelectCoins(const CAmount& nTargetValue, std::set<std::pair<const WalletTx*,unsigned int> >& setCoinsRet, CAmount& nValueRet, const CCoinControl* coinControl) const
{
    std::vector<COutput> vCoins;
    AvailableCoins(vCoins, true, coinControl);

    // coin control -> return all selected outputs (we want all selected to go into the transaction for sure)
    if (coinControl && coinControl->HasSelected() && !coinControl->fAllowOtherInputs)
    {
        BOOST_FOREACH(const COutput& out, vCoins)
        {
            if (!out.fSpendable)
                continue;
            nValueRet += out.tx->vout[out.i].nValue;
            setCoinsRet.insert(std::make_pair(out.tx, out.i));
        }
        return (nValueRet >= nTargetValue);
    }

    // calculate value from preset inputs and store them
    std::set<std::pair<const WalletTx*, uint32_t> > setPresetCoins;
    CAmount nValueFromPresetInputs = 0;

    std::vector<COutPoint> vPresetInputs;
    if (coinControl)
        coinControl->ListSelected(vPresetInputs);
    BOOST_FOREACH(const COutPoint& outpoint, vPresetInputs)
    {
        std::map<uint256, WalletTx>::const_iterator it = mapWallet.find(outpoint.hash);
        if (it != mapWallet.end())
        {
            const WalletTx* pcoin = &it->second;
            // Clearly invalid input, fail
            if (pcoin->vout.size() <= outpoint.n)
                return false;
            nValueFromPresetInputs += pcoin->vout[outpoint.n].nValue;
            setPresetCoins.insert(std::make_pair(pcoin, outpoint.n));
        } else
            return false; // TODO: Allow non-wallet inputs
    }
    
    // remove preset inputs from vCoins
    for (std::vector<COutput>::iterator it = vCoins.begin(); it != vCoins.end() && coinControl && coinControl->HasSelected();)
    {
        if (setPresetCoins.count(std::make_pair(it->tx, it->i)))
            it = vCoins.erase(it);
        else
            ++it;
    }
    
    bool res = nTargetValue <= nValueFromPresetInputs ||
    SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 1, 6, vCoins, setCoinsRet, nValueRet) ||
    SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 1, 1, vCoins, setCoinsRet, nValueRet) ||
    (bSpendZeroConfChange && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, 0, 1, vCoins, setCoinsRet, nValueRet));
    
    // because SelectCoinsMinConf clears the setCoinsRet, we now add the possible inputs to the coinset
    setCoinsRet.insert(setPresetCoins.begin(), setPresetCoins.end());
    
    // add preset inputs to the total value selected
    nValueRet += nValueFromPresetInputs;
    
    return res;
}

bool Wallet::CreateTransaction(const std::vector<CRecipient>& vecSend, WalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRet,
                                int& nChangePosRet, std::string& strFailReason, const CCoinControl* coinControl, bool sign)
{
    CAmount nValue = 0;
    unsigned int nSubtractFeeFromAmount = 0;
    CFeeRate minRelayTxFee = GetMinRelayTxFee();
    BOOST_FOREACH (const CRecipient& recipient, vecSend)
    {
        if (nValue < 0 || recipient.nAmount < 0)
        {
            strFailReason = _("Transaction amounts must be positive");
            return false;
        }
        nValue += recipient.nAmount;

        if (recipient.fSubtractFeeFromAmount)
            nSubtractFeeFromAmount++;
    }
    if (vecSend.empty() || nValue < 0)
    {
        strFailReason = _("Transaction amounts must be positive");
        return false;
    }

    wtxNew.fTimeReceivedIsTxTime = true;
    wtxNew.BindWallet(this);
    CMutableTransaction txNew;

    // Discourage fee sniping.
    //
    // However because of a off-by-one-error in previous versions we need to
    // neuter it by setting nLockTime to at least one less than nBestHeight.
    // Secondly currently propagation of transactions created for block heights
    // corresponding to blocks that were just mined may be iffy - transactions
    // aren't re-accepted into the mempool - we additionally neuter the code by
    // going ten blocks back. Doesn't yet do anything for sniping, but does act
    // to shake out wallet bugs like not showing nLockTime'd transactions at
    // all.
    int activeChainHeight = GetActiveChainHeight();

    txNew.nLockTime = std::max(0, activeChainHeight - 10);

    // Secondly occasionally randomly pick a nLockTime even further back, so
    // that transactions that are delayed after signing for whatever reason,
    // e.g. high-latency mix networks and some CoinJoin implementations, have
    // better privacy.
    if (GetRandInt(10) == 0)
        txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));

    assert(txNew.nLockTime <= (unsigned int)activeChainHeight);
    assert(txNew.nLockTime < LOCKTIME_THRESHOLD);

    {
        LOCK(cs_coreWallet);
        {
            nFeeRet = 0;
            while (true)
            {
                txNew.vin.clear();
                txNew.vout.clear();
                wtxNew.fFromMe = true;
                nChangePosRet = -1;
                bool fFirst = true;

                CAmount nTotalValue = nValue;
                if (nSubtractFeeFromAmount == 0)
                    nTotalValue += nFeeRet;
                double dPriority = 0;
                // vouts to the payees
                BOOST_FOREACH (const CRecipient& recipient, vecSend)
                {
                    CTxOut txout(recipient.nAmount, recipient.scriptPubKey);

                    if (recipient.fSubtractFeeFromAmount)
                    {
                        txout.nValue -= nFeeRet / nSubtractFeeFromAmount; // Subtract fee equally from each selected recipient

                        if (fFirst) // first receiver pays the remainder not divisible by output count
                        {
                            fFirst = false;
                            txout.nValue -= nFeeRet % nSubtractFeeFromAmount;
                        }
                    }

                    if (txout.IsDust(minRelayTxFee))
                    {
                        if (recipient.fSubtractFeeFromAmount && nFeeRet > 0)
                        {
                            if (txout.nValue < 0)
                                strFailReason = _("The transaction amount is too small to pay the fee");
                            else
                                strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
                        }
                        else
                            strFailReason = _("Transaction amount too small");
                        return false;
                    }
                    txNew.vout.push_back(txout);
                }

                // Choose coins to use
                std::set<std::pair<const WalletTx*,unsigned int> > setCoins;
                CAmount nValueIn = 0;
                if (!SelectCoins(nTotalValue, setCoins, nValueIn, coinControl))
                {
                    strFailReason = _("Insufficient funds");
                    return false;
                }
                BOOST_FOREACH(PAIRTYPE(const WalletTx*, unsigned int) pcoin, setCoins)
                {
                    CAmount nCredit = pcoin.first->vout[pcoin.second].nValue;
                    //The coin age after the next block (depth+1) is used instead of the current,
                    //reflecting an assumption the user would accept a bit more delay for
                    //a chance at a free transaction.
                    //But mempool inputs might still be in the mempool, so their age stays 0
                    int age = pcoin.first->GetDepthInMainChain();
                    if (age != 0)
                        age += 1;
                    dPriority += (double)nCredit * age;
                }

                CAmount nChange = nValueIn - nValue;
                if (nSubtractFeeFromAmount == 0)
                    nChange -= nFeeRet;

                if (nChange > 0)
                {
                    // Fill a vout to ourself
                    // TODO: pass in scriptChange instead of reservekey so
                    // change transaction isn't always pay-to-bitcoin-address
                    CScript scriptChange;

                    // coin control: send change to custom address
                    if (coinControl && !boost::get<CNoDestination>(&coinControl->destChange))
                        scriptChange = GetScriptForDestination(coinControl->destChange);

                    // no coin control: send change to newly generated address
                    else
                    {
                        // Note: We use a new key here to keep it from being obvious which side is the change.
                        //  The drawback is that by not reusing a previous key, the change may be lost if a
                        //  backup is restored, if the backup doesn't have the new private key for the change.
                        //  If we reused the old key, it would be possible to add code to look for and
                        //  rediscover unknown transactions that were written with keys of ours to recover
                        //  post-backup change.

                        // Reserve a new key pair from key pool
                        CPubKey vchPubKey;
                        bool ret;
                        ret = reservekey.GetReservedKey(vchPubKey);
                        assert(ret); // should never fail, as we just unlocked

                        scriptChange = GetScriptForDestination(vchPubKey.GetID());
                    }

                    CTxOut newTxOut(nChange, scriptChange);

                    // We do not move dust-change to fees, because the sender would end up paying more than requested.
                    // This would be against the purpose of the all-inclusive feature.
                    // So instead we raise the change and deduct from the recipient.
                    if (nSubtractFeeFromAmount > 0 && newTxOut.IsDust(minRelayTxFee))
                    {
                        CAmount nDust = newTxOut.GetDustThreshold(minRelayTxFee) - newTxOut.nValue;
                        newTxOut.nValue += nDust; // raise change until no more dust
                        for (unsigned int i = 0; i < vecSend.size(); i++) // subtract from first recipient
                        {
                            if (vecSend[i].fSubtractFeeFromAmount)
                            {
                                txNew.vout[i].nValue -= nDust;
                                if (txNew.vout[i].IsDust(minRelayTxFee))
                                {
                                    strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
                                    return false;
                                }
                                break;
                            }
                        }
                    }

                    // Never create dust outputs; if we would, just
                    // add the dust to the fee.
                    if (newTxOut.IsDust(minRelayTxFee))
                    {
                        nFeeRet += nChange;
                        reservekey.ReturnKey();
                    }
                    else
                    {
                        // Insert change txn at random position:
                        nChangePosRet = GetRandInt(txNew.vout.size()+1);
                        std::vector<CTxOut>::iterator position = txNew.vout.begin()+nChangePosRet;
                        txNew.vout.insert(position, newTxOut);
                    }
                }
                else
                    reservekey.ReturnKey();

                // Fill vin
                //
                // Note how the sequence number is set to max()-1 so that the
                // nLockTime set above actually works.
                BOOST_FOREACH(const PAIRTYPE(const WalletTx*,unsigned int)& coin, setCoins)
                txNew.vin.push_back(CTxIn(coin.first->GetHash(),coin.second,CScript(),
                                          std::numeric_limits<unsigned int>::max()-1));

                // Sign
                int nIn = 0;
                CTransaction txNewConst(txNew);
                BOOST_FOREACH(const PAIRTYPE(const WalletTx*,unsigned int)& coin, setCoins)
                {
                    bool signSuccess;
                    const CScript& scriptPubKey = coin.first->vout[coin.second].scriptPubKey;
                    CScript& scriptSigRes = txNew.vin[nIn].scriptSig;
                    if (sign)
                        signSuccess = ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, SIGHASH_ALL), scriptPubKey, scriptSigRes);
                    else
                        signSuccess = ProduceSignature(DummySignatureCreator(this), scriptPubKey, scriptSigRes);

                    if (!signSuccess)
                    {
                        strFailReason = _("Signing transaction failed");
                        return false;
                    }
                    nIn++;
                }

                unsigned int nBytes = ::GetSerializeSize(txNew, SER_NETWORK, PROTOCOL_VERSION);

                // Remove scriptSigs if we used dummy signatures for fee calculation
                if (!sign) {
                    BOOST_FOREACH (CTxIn& vin, txNew.vin)
                    vin.scriptSig = CScript();
                }

                // Embed the constructed transaction data in wtxNew.
                *static_cast<CTransaction*>(&wtxNew) = CTransaction(txNew);

                // Limit size
                if (nBytes >= GetMaxStandardTxSize())
                {
                    strFailReason = _("Transaction too large");
                    return false;
                }

                dPriority = wtxNew.ComputePriority(dPriority, nBytes);

                // Can we complete this as a free transaction?
                if (fSendFreeTransactions && nBytes <= MAX_FREE_TRANSACTION_CREATE_SIZE)
                {
                    // Not enough fee: enough priority?
                    double dPriorityNeeded = coreInterface.EstimatePriority(nTxConfirmTarget);

                    // Not enough mempool history to estimate: use hard-coded AllowFree.
                    if (dPriorityNeeded <= 0 && coreInterface.AllowFree(dPriority))
                        break;
                    
                    // Small enough, and priority high enough, to send for free
                    if (dPriorityNeeded > 0 && dPriority >= dPriorityNeeded)
                        break;
                }
                
                CAmount nFeeNeeded = GetMinimumFee(nBytes, nTxConfirmTarget);
                
                // If we made it here and we aren't even able to meet the relay fee on the next pass, give up
                // because we must be at the maximum allowed fee.
                if (nFeeNeeded < minRelayTxFee.GetFee(nBytes))
                {
                    strFailReason = _("Transaction too large for fee policy");
                    return false;
                }
                
                if (nFeeRet >= nFeeNeeded)
                    break; // Done, enough fee included.
                
                // Include more fee and try again.
                nFeeRet = nFeeNeeded;
                continue;
            }
        }
    }
    
    return true;
}

/**
 * Call after CreateTransaction unless you want to abort
 */
bool Wallet::CommitTransaction(WalletTx& wtxNew, CReserveKey& reservekey)
{
    LogPrintf("CommitTransaction:\n%s", wtxNew.ToString());

    // Take key pair from key pool so it won't be used again
    reservekey.KeepKey();

    // Add tx to wallet, because if it has change it's also ours,
    // otherwise just for transaction history.
    AddToWallet(wtxNew, false);

    // Notify that old coins are spent
    {
        LOCK(cs_coreWallet);

        std::set<WalletTx*> setCoins;
        BOOST_FOREACH(const CTxIn& txin, wtxNew.vin)
        {
            WalletTx &coin = mapWallet[txin.prevout.hash];
            coin.BindWallet(this);
            NotifyTransactionChanged(this, coin.GetHash(), CT_UPDATED);
        }

        // Track how many getdata requests our transaction gets
        mapRequestCount[wtxNew.GetHash()] = 0;
    }

    if (fBroadcastTransactions)
    {
        // Broadcast
        if (!AcceptToMemoryPool(wtxNew, false))
        {
            // This must not fail. The transaction has already been signed and recorded.
            LogPrintf("CommitTransaction(): Error: Transaction not valid");
            return false;
        }
        RelayWalletTransaction(wtxNew);
    }
    return true;
}

CAmount Wallet::GetMinimumFee(unsigned int nTxBytes, unsigned int nConfirmTarget)
{
    // payTxFee is user-set "I want to pay this much"
    CAmount nFeeNeeded = payTxFee.GetFee(nTxBytes);
    // user selected total at least (default=true)
    if (fPayAtLeastCustomFee && nFeeNeeded > 0 && nFeeNeeded < payTxFee.GetFeePerK())
        nFeeNeeded = payTxFee.GetFeePerK();
    // User didn't set: use -txconfirmtarget to estimate...
    if (nFeeNeeded == 0)
    {
        LOCK(cs_coreWallet);
        nFeeNeeded = coreInterface.EstimateFee(nConfirmTarget, nTxBytes);
    }
    // ... unless we don't have enough mempool data, in which case fall
    // back to a hard-coded fee
    if (nFeeNeeded == 0)
        nFeeNeeded = minTxFee.GetFee(nTxBytes);
    // prevent user from paying a non-sense fee (like 1 satoshi): 0 < fee < minRelayFee
    CFeeRate minRelayTxFee = GetMinRelayTxFee();
    if (nFeeNeeded < minRelayTxFee.GetFee(nTxBytes))
        nFeeNeeded = minRelayTxFee.GetFee(nTxBytes);
    // But always obey the maximum
    if (nFeeNeeded > maxTxFee)
        nFeeNeeded = maxTxFee;
    return nFeeNeeded;
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

bool Wallet::AddToWallet(const WalletTx& wtxIn, bool fOnlyInMemory)
{
    LOCK(cs_coreWallet);

    uint256 hash = wtxIn.GetHash();

    if (fOnlyInMemory)
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
                        WtxItems txOrdered = OrderedTxItems();
                        for (WtxItems::reverse_iterator it = txOrdered.rbegin(); it != txOrdered.rend(); ++it)
                        {
                            WalletTx *const pwtx = (*it).second;
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

            CAmount debit = 0;
            if (prev.GetCache(CREDIT_DEBIT_TYPE_DEBIT, filter, debit))
                return debit;

            if (txin.prevout.n < prev.vout.size())
                if (IsMine(prev.vout[txin.prevout.n]) & filter)
                {
                    debit = prev.vout[txin.prevout.n].nValue;
                    prev.SetCache(CREDIT_DEBIT_TYPE_DEBIT, filter, debit);
                    return debit;
                }
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

bool Wallet::IsFromMe(const CTransaction& tx, const isminefilter& filter) const
{
    return (GetDebit(tx, filter) > 0);
}

bool Wallet::WriteWTXToDisk(const WalletTx &wtx)
{
    return walletCacheDB->WriteTx(wtx.GetHash(), wtx);
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
 # Conflict management stack #
 #############################
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
    LOCK(cs_coreWallet);

    CAmount nTotal = 0;
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
                    continue;
                }

                CAmount nCredit = GetCredit(*pcoin, filter);
                pcoin->SetCache(balanceType, filter, nCredit);
                nTotal += nCredit;
                continue;
            }

            // Must wait until coinbase is safely deep enough in the chain before valuing it
            if (pcoin->IsCoinBase() && pcoin->GetBlocksToMaturity() > 0)
                continue;

            if (cacheFound)
            {
                nTotal += cacheOut;
                continue;
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

CFeeRate Wallet::GetMinRelayTxFee()
{
    LOCK(cs_coreWallet);
    return coreInterface.GetMinRelayTxFee();
}

unsigned int Wallet::GetMaxStandardTxSize()
{
    LOCK(cs_coreWallet);
    return coreInterface.GetMaxStandardTxSize();
}

void Wallet::GetScriptForMining(boost::shared_ptr<CReserveScript> &script)
{
    //used for mining (mainly for regression tests)
    boost::shared_ptr<CHDReserveKey> rKey(new CHDReserveKey(this));
    CPubKey pubkey;
    if (!rKey->GetReservedKey(pubkey))
        return;

    script = rKey;
    script->reserveScript = CScript() << ToByteVector(pubkey) << OP_CHECKSIG;
}

/*
###########################
# Reserve & Keypool Stack #
###########################
*/
bool CReserveKey::GetReservedKey(CPubKey& pubkey)
{
//    if (nIndex == -1)
//    {
//        CKeyPool keypool;
//        pwallet->ReserveKeyFromKeyPool(nIndex, keypool);
//        if (nIndex != -1)
//            vchPubKey = keypool.vchPubKey;
//        else {
//            return false;
//        }
//    }
//    assert(vchPubKey.IsValid());
//    pubkey = vchPubKey;
    return true;
}

void CReserveKey::KeepKey()
{
//    if (nIndex != -1)
//        pwallet->KeepKey(nIndex);
//    nIndex = -1;
//    vchPubKey = CPubKey();
}

void CReserveKey::ReturnKey()
{
//    if (nIndex != -1)
//        pwallet->ReturnKey(nIndex);
//    nIndex = -1;
//    vchPubKey = CPubKey();
}

bool CHDReserveKey::GetReservedKey(CPubKey& pubkey)
{
    std::string newKeysChainpath;
    LOCK(pwallet->cs_coreWallet);
    return pwallet->HDGetNextChildPubKey(chainID, pubkey, newKeysChainpath, true);
}

void CHDReserveKey::KeepKey()
{

}

void CHDReserveKey::ReturnKey()
{
    //TODO: implement a way of release the nChild index of a returned HDReserveKey
    //at the moment there is no way of returning HD change keys
}
    
}; //end namespace