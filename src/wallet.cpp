// Copyright (c) 2009-2011 Satoshi Nakamoto & Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "headers.h"
#include "db.h"
#include "cryptopp/sha.h"

using namespace std;



//////////////////////////////////////////////////////////////////////////////
//
// mapWallet
//

bool CWallet::AddKey(const CKey& key)
{
    this->CKeyStore::AddKey(key);
    if (!fFileBacked)
        return true;
    return CWalletDB(strWalletFile).WriteKey(key.GetPubKey(), key.GetPrivKey());
}

void CWallet::WalletUpdateSpent(const CTransaction &tx)
{
    // Anytime a signature is successfully verified, it's proof the outpoint is spent.
    // Update the wallet spent flag if it doesn't know due to wallet.dat being
    // restored from backup or the user making copies of wallet.dat.
    CRITICAL_BLOCK(cs_mapWallet)
    {
        BOOST_FOREACH(const CTxIn& txin, tx.vin)
        {
            map<uint256, CWalletTx>::iterator mi = mapWallet.find(txin.prevout.hash);
            if (mi != mapWallet.end())
            {
                CWalletTx& wtx = (*mi).second;
                if (!wtx.IsSpent(txin.prevout.n) && IsMine(wtx.vout[txin.prevout.n]))
                {
                    printf("WalletUpdateSpent found spent coin %sbc %s\n", FormatMoney(wtx.GetCredit()).c_str(), wtx.GetHash().ToString().c_str());
                    wtx.MarkSpent(txin.prevout.n);
                    wtx.WriteToDisk();
                    vWalletUpdated.push_back(txin.prevout.hash);
                }
            }
        }
    }
}

bool CWallet::AddToWallet(const CWalletTx& wtxIn)
{
    uint256 hash = wtxIn.GetHash();
    CRITICAL_BLOCK(cs_mapWallet)
    {
        // Inserts only if not already there, returns tx inserted or tx found
        pair<map<uint256, CWalletTx>::iterator, bool> ret = mapWallet.insert(make_pair(hash, wtxIn));
        CWalletTx& wtx = (*ret.first).second;
        wtx.pwallet = this;
        bool fInsertedNew = ret.second;
        if (fInsertedNew)
            wtx.nTimeReceived = GetAdjustedTime();

        bool fUpdated = false;
        if (!fInsertedNew)
        {
            // Merge
            if (wtxIn.hashBlock != 0 && wtxIn.hashBlock != wtx.hashBlock)
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
            fUpdated |= wtx.UpdateSpent(wtxIn.vfSpent);
        }

        //// debug print
        printf("AddToWallet %s  %s%s\n", wtxIn.GetHash().ToString().substr(0,10).c_str(), (fInsertedNew ? "new" : ""), (fUpdated ? "update" : ""));

        // Write to disk
        if (fInsertedNew || fUpdated)
            if (!wtx.WriteToDisk())
                return false;

        // If default receiving address gets used, replace it with a new one
        CScript scriptDefaultKey;
        scriptDefaultKey.SetBitcoinAddress(vchDefaultKey);
        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
        {
            if (txout.scriptPubKey == scriptDefaultKey)
                SetDefaultKey(GetKeyFromKeyPool());
        }

        // Notify UI
        vWalletUpdated.push_back(hash);

        // since AddToWallet is called directly for self-originating transactions, check for consumption of own coins
        WalletUpdateSpent(wtx);
    }

    // Refresh UI
    MainFrameRepaint();
    return true;
}

bool CWallet::AddToWalletIfInvolvingMe(const CTransaction& tx, const CBlock* pblock, bool fUpdate)
{
    uint256 hash = tx.GetHash();
    bool fExisted = mapWallet.count(hash);
    if (fExisted && !fUpdate) return false;
    if (fExisted || IsMine(tx) || IsFromMe(tx))
    {
        CWalletTx wtx(this,tx);
        // Get merkle branch if transaction was found in a block
        if (pblock)
            wtx.SetMerkleBranch(pblock);
        return AddToWallet(wtx);
    }
    else
        WalletUpdateSpent(tx);
    return false;
}

bool CWallet::EraseFromWallet(uint256 hash)
{
    if (!fFileBacked)
        return false;
    CRITICAL_BLOCK(cs_mapWallet)
    {
        if (mapWallet.erase(hash))
            CWalletDB(strWalletFile).EraseTx(hash);
    }
    return true;
}


bool CWallet::IsMine(const CTxIn &txin) const
{
    CRITICAL_BLOCK(cs_mapWallet)
    {
        map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(txin.prevout.hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx& prev = (*mi).second;
            if (txin.prevout.n < prev.vout.size())
                if (IsMine(prev.vout[txin.prevout.n]))
                    return true;
        }
    }
    return false;
}

int64 CWallet::GetDebit(const CTxIn &txin) const
{
    CRITICAL_BLOCK(cs_mapWallet)
    {
        map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(txin.prevout.hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx& prev = (*mi).second;
            if (txin.prevout.n < prev.vout.size())
                if (IsMine(prev.vout[txin.prevout.n]))
                    return prev.vout[txin.prevout.n].nValue;
        }
    }
    return 0;
}

int64 CWalletTx::GetTxTime() const
{
    if (!fTimeReceivedIsTxTime && hashBlock != 0)
    {
        // If we did not receive the transaction directly, we rely on the block's
        // time to figure out when it happened.  We use the median over a range
        // of blocks to try to filter out inaccurate block times.
        map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
        if (mi != mapBlockIndex.end())
        {
            CBlockIndex* pindex = (*mi).second;
            if (pindex)
                return pindex->GetMedianTime();
        }
    }
    return nTimeReceived;
}

int CWalletTx::GetRequestCount() const
{
    // Returns -1 if it wasn't being tracked
    int nRequests = -1;
    CRITICAL_BLOCK(pwallet->cs_mapRequestCount)
    {
        if (IsCoinBase())
        {
            // Generated block
            if (hashBlock != 0)
            {
                map<uint256, int>::const_iterator mi = pwallet->mapRequestCount.find(hashBlock);
                if (mi != pwallet->mapRequestCount.end())
                    nRequests = (*mi).second;
            }
        }
        else
        {
            // Did anyone request this transaction?
            map<uint256, int>::const_iterator mi = pwallet->mapRequestCount.find(GetHash());
            if (mi != pwallet->mapRequestCount.end())
            {
                nRequests = (*mi).second;

                // How about the block it's in?
                if (nRequests == 0 && hashBlock != 0)
                {
                    map<uint256, int>::const_iterator mi = pwallet->mapRequestCount.find(hashBlock);
                    if (mi != pwallet->mapRequestCount.end())
                        nRequests = (*mi).second;
                    else
                        nRequests = 1; // If it's in someone else's block it must have got out
                }
            }
        }
    }
    return nRequests;
}

void CWalletTx::GetAmounts(int64& nGeneratedImmature, int64& nGeneratedMature, list<pair<string, int64> >& listReceived,
                           list<pair<string, int64> >& listSent, int64& nFee, string& strSentAccount) const
{
    nGeneratedImmature = nGeneratedMature = nFee = 0;
    listReceived.clear();
    listSent.clear();
    strSentAccount = strFromAccount;

    if (IsCoinBase())
    {
        if (GetBlocksToMaturity() > 0)
            nGeneratedImmature = pwallet->GetCredit(*this);
        else
            nGeneratedMature = GetCredit();
        return;
    }

    // Compute fee:
    int64 nDebit = GetDebit();
    if (nDebit > 0) // debit>0 means we signed/sent this transaction
    {
        int64 nValueOut = GetValueOut();
        nFee = nDebit - nValueOut;
    }

    // Sent/received.  Standard client will never generate a send-to-multiple-recipients,
    // but non-standard clients might (so return a list of address/amount pairs)
    BOOST_FOREACH(const CTxOut& txout, vout)
    {
        string address;
        uint160 hash160;
        vector<unsigned char> vchPubKey;
        if (ExtractHash160(txout.scriptPubKey, hash160))
            address = Hash160ToAddress(hash160);
        else if (ExtractPubKey(txout.scriptPubKey, NULL, vchPubKey))
            address = PubKeyToAddress(vchPubKey);
        else
        {
            printf("CWalletTx::GetAmounts: Unknown transaction type found, txid %s\n",
                   this->GetHash().ToString().c_str());
            address = " unknown ";
        }

        // Don't report 'change' txouts
        if (nDebit > 0 && pwallet->IsChange(txout))
            continue;

        if (nDebit > 0)
            listSent.push_back(make_pair(address, txout.nValue));

        if (pwallet->IsMine(txout))
            listReceived.push_back(make_pair(address, txout.nValue));
    }

}

void CWalletTx::GetAccountAmounts(const string& strAccount, int64& nGenerated, int64& nReceived, 
                                  int64& nSent, int64& nFee) const
{
    nGenerated = nReceived = nSent = nFee = 0;

    int64 allGeneratedImmature, allGeneratedMature, allFee;
    allGeneratedImmature = allGeneratedMature = allFee = 0;
    string strSentAccount;
    list<pair<string, int64> > listReceived;
    list<pair<string, int64> > listSent;
    GetAmounts(allGeneratedImmature, allGeneratedMature, listReceived, listSent, allFee, strSentAccount);

    if (strAccount == "")
        nGenerated = allGeneratedMature;
    if (strAccount == strSentAccount)
    {
        BOOST_FOREACH(const PAIRTYPE(string,int64)& s, listSent)
            nSent += s.second;
        nFee = allFee;
    }
    CRITICAL_BLOCK(pwallet->cs_mapAddressBook)
    {
        BOOST_FOREACH(const PAIRTYPE(string,int64)& r, listReceived)
        {
            if (pwallet->mapAddressBook.count(r.first))
            {
                map<string, string>::const_iterator mi = pwallet->mapAddressBook.find(r.first);
                if (mi != pwallet->mapAddressBook.end() && (*mi).second == strAccount)
                    nReceived += r.second;
            }
            else if (strAccount.empty())
            {
                nReceived += r.second;
            }
        }
    }
}

void CWalletTx::AddSupportingTransactions(CTxDB& txdb)
{
    vtxPrev.clear();

    const int COPY_DEPTH = 3;
    if (SetMerkleBranch() < COPY_DEPTH)
    {
        vector<uint256> vWorkQueue;
        BOOST_FOREACH(const CTxIn& txin, vin)
            vWorkQueue.push_back(txin.prevout.hash);

        // This critsect is OK because txdb is already open
        CRITICAL_BLOCK(pwallet->cs_mapWallet)
        {
            map<uint256, const CMerkleTx*> mapWalletPrev;
            set<uint256> setAlreadyDone;
            for (int i = 0; i < vWorkQueue.size(); i++)
            {
                uint256 hash = vWorkQueue[i];
                if (setAlreadyDone.count(hash))
                    continue;
                setAlreadyDone.insert(hash);

                CMerkleTx tx;
                map<uint256, CWalletTx>::const_iterator mi = pwallet->mapWallet.find(hash);
                if (mi != pwallet->mapWallet.end())
                {
                    tx = (*mi).second;
                    BOOST_FOREACH(const CMerkleTx& txWalletPrev, (*mi).second.vtxPrev)
                        mapWalletPrev[txWalletPrev.GetHash()] = &txWalletPrev;
                }
                else if (mapWalletPrev.count(hash))
                {
                    tx = *mapWalletPrev[hash];
                }
                else if (!fClient && txdb.ReadDiskTx(hash, tx))
                {
                    ;
                }
                else
                {
                    printf("ERROR: AddSupportingTransactions() : unsupported transaction\n");
                    continue;
                }

                int nDepth = tx.SetMerkleBranch();
                vtxPrev.push_back(tx);

                if (nDepth < COPY_DEPTH)
                    BOOST_FOREACH(const CTxIn& txin, tx.vin)
                        vWorkQueue.push_back(txin.prevout.hash);
            }
        }
    }

    reverse(vtxPrev.begin(), vtxPrev.end());
}

bool CWalletTx::WriteToDisk()
{
    return CWalletDB(pwallet->strWalletFile).WriteTx(GetHash(), *this);
}

int CWallet::ScanForWalletTransactions(CBlockIndex* pindexStart, bool fUpdate)
{
    int ret = 0;

    CBlockIndex* pindex = pindexStart;
    CRITICAL_BLOCK(cs_mapWallet)
    {
        while (pindex)
        {
            CBlock block;
            block.ReadFromDisk(pindex, true);
            BOOST_FOREACH(CTransaction& tx, block.vtx)
            {
                if (AddToWalletIfInvolvingMe(tx, &block, fUpdate))
                    ret++;
            }
            pindex = pindex->pnext;
        }
    }
    return ret;
}

void CWallet::ReacceptWalletTransactions()
{
    CTxDB txdb("r");
    bool fRepeat = true;
    while (fRepeat) CRITICAL_BLOCK(cs_mapWallet)
    {
        fRepeat = false;
        vector<CDiskTxPos> vMissingTx;
        BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, mapWallet)
        {
            CWalletTx& wtx = item.second;
            if (wtx.IsCoinBase() && wtx.IsSpent(0))
                continue;

            CTxIndex txindex;
            bool fUpdated = false;
            if (txdb.ReadTxIndex(wtx.GetHash(), txindex))
            {
                // Update fSpent if a tx got spent somewhere else by a copy of wallet.dat
                if (txindex.vSpent.size() != wtx.vout.size())
                {
                    printf("ERROR: ReacceptWalletTransactions() : txindex.vSpent.size() %d != wtx.vout.size() %d\n", txindex.vSpent.size(), wtx.vout.size());
                    continue;
                }
                for (int i = 0; i < txindex.vSpent.size(); i++)
                {
                    if (wtx.IsSpent(i))
                        continue;
                    if (!txindex.vSpent[i].IsNull() && IsMine(wtx.vout[i]))
                    {
                        wtx.MarkSpent(i);
                        fUpdated = true;
                        vMissingTx.push_back(txindex.vSpent[i]);
                    }
                }
                if (fUpdated)
                {
                    printf("ReacceptWalletTransactions found spent coin %sbc %s\n", FormatMoney(wtx.GetCredit()).c_str(), wtx.GetHash().ToString().c_str());
                    wtx.MarkDirty();
                    wtx.WriteToDisk();
                }
            }
            else
            {
                // Reaccept any txes of ours that aren't already in a block
                if (!wtx.IsCoinBase())
                    wtx.AcceptWalletTransaction(txdb, false);
            }
        }
        if (!vMissingTx.empty())
        {
            // TODO: optimize this to scan just part of the block chain?
            if (ScanForWalletTransactions(pindexGenesisBlock))
                fRepeat = true;  // Found missing transactions: re-do Reaccept.
        }
    }
}

void CWalletTx::RelayWalletTransaction(CTxDB& txdb)
{
    BOOST_FOREACH(const CMerkleTx& tx, vtxPrev)
    {
        if (!tx.IsCoinBase())
        {
            uint256 hash = tx.GetHash();
            if (!txdb.ContainsTx(hash))
                RelayMessage(CInv(MSG_TX, hash), (CTransaction)tx);
        }
    }
    if (!IsCoinBase())
    {
        uint256 hash = GetHash();
        if (!txdb.ContainsTx(hash))
        {
            printf("Relaying wtx %s\n", hash.ToString().substr(0,10).c_str());
            RelayMessage(CInv(MSG_TX, hash), (CTransaction)*this);
        }
    }
}

void CWalletTx::RelayWalletTransaction()
{
   CTxDB txdb("r");
   RelayWalletTransaction(txdb);
}

void CWallet::ResendWalletTransactions()
{
    // Do this infrequently and randomly to avoid giving away
    // that these are our transactions.
    static int64 nNextTime;
    if (GetTime() < nNextTime)
        return;
    bool fFirst = (nNextTime == 0);
    nNextTime = GetTime() + GetRand(30 * 60);
    if (fFirst)
        return;

    // Only do it if there's been a new block since last time
    static int64 nLastTime;
    if (nTimeBestReceived < nLastTime)
        return;
    nLastTime = GetTime();

    // Rebroadcast any of our txes that aren't in a block yet
    printf("ResendWalletTransactions()\n");
    CTxDB txdb("r");
    CRITICAL_BLOCK(cs_mapWallet)
    {
        // Sort them in chronological order
        multimap<unsigned int, CWalletTx*> mapSorted;
        BOOST_FOREACH(PAIRTYPE(const uint256, CWalletTx)& item, mapWallet)
        {
            CWalletTx& wtx = item.second;
            // Don't rebroadcast until it's had plenty of time that
            // it should have gotten in already by now.
            if (nTimeBestReceived - (int64)wtx.nTimeReceived > 5 * 60)
                mapSorted.insert(make_pair(wtx.nTimeReceived, &wtx));
        }
        BOOST_FOREACH(PAIRTYPE(const unsigned int, CWalletTx*)& item, mapSorted)
        {
            CWalletTx& wtx = *item.second;
            wtx.RelayWalletTransaction(txdb);
        }
    }
}






//////////////////////////////////////////////////////////////////////////////
//
// Actions
//


int64 CWallet::GetBalance() const
{
    int64 nStart = GetTimeMillis();

    int64 nTotal = 0;
    CRITICAL_BLOCK(cs_mapWallet)
    {
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;
            if (!pcoin->IsFinal() || !pcoin->IsConfirmed())
                continue;
            nTotal += pcoin->GetAvailableCredit();
        }
    }

    //printf("GetBalance() %"PRI64d"ms\n", GetTimeMillis() - nStart);
    return nTotal;
}


bool CWallet::SelectCoinsMinConf(int64 nTargetValue, int nConfMine, int nConfTheirs, set<pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64& nValueRet) const
{
    setCoinsRet.clear();
    nValueRet = 0;

    // List of values less than target
    pair<int64, pair<const CWalletTx*,unsigned int> > coinLowestLarger;
    coinLowestLarger.first = INT64_MAX;
    coinLowestLarger.second.first = NULL;
    vector<pair<int64, pair<const CWalletTx*,unsigned int> > > vValue;
    int64 nTotalLower = 0;

    CRITICAL_BLOCK(cs_mapWallet)
    {
       vector<const CWalletTx*> vCoins;
       vCoins.reserve(mapWallet.size());
       for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
           vCoins.push_back(&(*it).second);
       random_shuffle(vCoins.begin(), vCoins.end(), GetRandInt);

       BOOST_FOREACH(const CWalletTx* pcoin, vCoins)
       {
            if (!pcoin->IsFinal() || !pcoin->IsConfirmed())
                continue;

            if (pcoin->IsCoinBase() && pcoin->GetBlocksToMaturity() > 0)
                continue;

            int nDepth = pcoin->GetDepthInMainChain();
            if (nDepth < (pcoin->IsFromMe() ? nConfMine : nConfTheirs))
                continue;

            for (int i = 0; i < pcoin->vout.size(); i++)
            {
                if (pcoin->IsSpent(i) || !IsMine(pcoin->vout[i]))
                    continue;

                int64 n = pcoin->vout[i].nValue;

                if (n <= 0)
                    continue;

                pair<int64,pair<const CWalletTx*,unsigned int> > coin = make_pair(n,make_pair(pcoin,i));

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
        }
    }

    if (nTotalLower == nTargetValue || nTotalLower == nTargetValue + CENT)
    {
        for (int i = 0; i < vValue.size(); ++i)
        {
            setCoinsRet.insert(vValue[i].second);
            nValueRet += vValue[i].first;
        }
        return true;
    }

    if (nTotalLower < nTargetValue + (coinLowestLarger.second.first ? CENT : 0))
    {
        if (coinLowestLarger.second.first == NULL)
            return false;
        setCoinsRet.insert(coinLowestLarger.second);
        nValueRet += coinLowestLarger.first;
        return true;
    }

    if (nTotalLower >= nTargetValue + CENT)
        nTargetValue += CENT;

    // Solve subset sum by stochastic approximation
    sort(vValue.rbegin(), vValue.rend());
    vector<char> vfIncluded;
    vector<char> vfBest(vValue.size(), true);
    int64 nBest = nTotalLower;

    for (int nRep = 0; nRep < 1000 && nBest != nTargetValue; nRep++)
    {
        vfIncluded.assign(vValue.size(), false);
        int64 nTotal = 0;
        bool fReachedTarget = false;
        for (int nPass = 0; nPass < 2 && !fReachedTarget; nPass++)
        {
            for (int i = 0; i < vValue.size(); i++)
            {
                if (nPass == 0 ? rand() % 2 : !vfIncluded[i])
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

    // If the next larger is still closer, return it
    if (coinLowestLarger.second.first && coinLowestLarger.first - nTargetValue <= nBest - nTargetValue)
    {
        setCoinsRet.insert(coinLowestLarger.second);
        nValueRet += coinLowestLarger.first;
    }
    else {
        for (int i = 0; i < vValue.size(); i++)
            if (vfBest[i])
            {
                setCoinsRet.insert(vValue[i].second);
                nValueRet += vValue[i].first;
            }

        //// debug print
        printf("SelectCoins() best subset: ");
        for (int i = 0; i < vValue.size(); i++)
            if (vfBest[i])
                printf("%s ", FormatMoney(vValue[i].first).c_str());
        printf("total %s\n", FormatMoney(nBest).c_str());
    }

    return true;
}

bool CWallet::SelectCoins(int64 nTargetValue, set<pair<const CWalletTx*,unsigned int> >& setCoinsRet, int64& nValueRet) const
{
    return (SelectCoinsMinConf(nTargetValue, 1, 6, setCoinsRet, nValueRet) ||
            SelectCoinsMinConf(nTargetValue, 1, 1, setCoinsRet, nValueRet) ||
            SelectCoinsMinConf(nTargetValue, 0, 1, setCoinsRet, nValueRet));
}




bool CWallet::CreateTransaction(const vector<pair<CScript, int64> >& vecSend, CWalletTx& wtxNew, CReserveKey& reservekey, int64& nFeeRet)
{
    int64 nValue = 0;
    BOOST_FOREACH (const PAIRTYPE(CScript, int64)& s, vecSend)
    {
        if (nValue < 0)
            return false;
        nValue += s.second;
    }
    if (vecSend.empty() || nValue < 0)
        return false;

    wtxNew.pwallet = this;

    CRITICAL_BLOCK(cs_main)
    {
        // txdb must be opened before the mapWallet lock
        CTxDB txdb("r");
        CRITICAL_BLOCK(cs_mapWallet)
        {
            nFeeRet = nTransactionFee;
            loop
            {
                wtxNew.vin.clear();
                wtxNew.vout.clear();
                wtxNew.fFromMe = true;

                int64 nTotalValue = nValue + nFeeRet;
                double dPriority = 0;
                // vouts to the payees
                BOOST_FOREACH (const PAIRTYPE(CScript, int64)& s, vecSend)
                    wtxNew.vout.push_back(CTxOut(s.second, s.first));

                // Choose coins to use
                set<pair<const CWalletTx*,unsigned int> > setCoins;
                int64 nValueIn = 0;
                if (!SelectCoins(nTotalValue, setCoins, nValueIn))
                    return false;
                BOOST_FOREACH(PAIRTYPE(const CWalletTx*, unsigned int) pcoin, setCoins)
                {
                    int64 nCredit = pcoin.first->vout[pcoin.second].nValue;
                    dPriority += (double)nCredit * pcoin.first->GetDepthInMainChain();
                }

                // Fill a vout back to self with any change
                int64 nChange = nValueIn - nTotalValue;
                if (nChange >= CENT)
                {
                    // Note: We use a new key here to keep it from being obvious which side is the change.
                    //  The drawback is that by not reusing a previous key, the change may be lost if a
                    //  backup is restored, if the backup doesn't have the new private key for the change.
                    //  If we reused the old key, it would be possible to add code to look for and
                    //  rediscover unknown transactions that were written with keys of ours to recover
                    //  post-backup change.

                    // Reserve a new key pair from key pool
                    vector<unsigned char> vchPubKey = reservekey.GetReservedKey();
                    assert(mapKeys.count(vchPubKey));

                    // Fill a vout to ourself, using same address type as the payment
                    CScript scriptChange;
                    if (vecSend[0].first.GetBitcoinAddressHash160() != 0)
                        scriptChange.SetBitcoinAddress(vchPubKey);
                    else
                        scriptChange << vchPubKey << OP_CHECKSIG;

                    // Insert change txn at random position:
                    vector<CTxOut>::iterator position = wtxNew.vout.begin()+GetRandInt(wtxNew.vout.size());
                    wtxNew.vout.insert(position, CTxOut(nChange, scriptChange));
                }
                else
                    reservekey.ReturnKey();

                // Fill vin
                BOOST_FOREACH(const PAIRTYPE(const CWalletTx*,unsigned int)& coin, setCoins)
                    wtxNew.vin.push_back(CTxIn(coin.first->GetHash(),coin.second));

                // Sign
                int nIn = 0;
                BOOST_FOREACH(const PAIRTYPE(const CWalletTx*,unsigned int)& coin, setCoins)
                    if (!SignSignature(*this, *coin.first, wtxNew, nIn++))
                        return false;

                // Limit size
                unsigned int nBytes = ::GetSerializeSize(*(CTransaction*)&wtxNew, SER_NETWORK);
                if (nBytes >= MAX_BLOCK_SIZE_GEN/5)
                    return false;
                dPriority /= nBytes;

                // Check that enough fee is included
                int64 nPayFee = nTransactionFee * (1 + (int64)nBytes / 1000);
                bool fAllowFree = CTransaction::AllowFree(dPriority);
                int64 nMinFee = wtxNew.GetMinFee(1, fAllowFree);
                if (nFeeRet < max(nPayFee, nMinFee))
                {
                    nFeeRet = max(nPayFee, nMinFee);
                    continue;
                }

                // Fill vtxPrev by copying from previous transactions vtxPrev
                wtxNew.AddSupportingTransactions(txdb);
                wtxNew.fTimeReceivedIsTxTime = true;

                break;
            }
        }
    }
    return true;
}

bool CWallet::CreateTransaction(CScript scriptPubKey, int64 nValue, CWalletTx& wtxNew, CReserveKey& reservekey, int64& nFeeRet)
{
    vector< pair<CScript, int64> > vecSend;
    vecSend.push_back(make_pair(scriptPubKey, nValue));
    return CreateTransaction(vecSend, wtxNew, reservekey, nFeeRet);
}

// Call after CreateTransaction unless you want to abort
bool CWallet::CommitTransaction(CWalletTx& wtxNew, CReserveKey& reservekey)
{
    CRITICAL_BLOCK(cs_main)
    {
        printf("CommitTransaction:\n%s", wtxNew.ToString().c_str());
        CRITICAL_BLOCK(cs_mapWallet)
        {
            // This is only to keep the database open to defeat the auto-flush for the
            // duration of this scope.  This is the only place where this optimization
            // maybe makes sense; please don't do it anywhere else.
            CWalletDB* pwalletdb = fFileBacked ? new CWalletDB(strWalletFile,"r") : NULL;

            // Take key pair from key pool so it won't be used again
            reservekey.KeepKey();

            // Add tx to wallet, because if it has change it's also ours,
            // otherwise just for transaction history.
            AddToWallet(wtxNew);

            // Mark old coins as spent
            set<CWalletTx*> setCoins;
            BOOST_FOREACH(const CTxIn& txin, wtxNew.vin)
            {
                CWalletTx &coin = mapWallet[txin.prevout.hash];
                coin.pwallet = this;
                coin.MarkSpent(txin.prevout.n);
                coin.WriteToDisk();
                vWalletUpdated.push_back(coin.GetHash());
            }

            if (fFileBacked)
                delete pwalletdb;
        }

        // Track how many getdata requests our transaction gets
        CRITICAL_BLOCK(cs_mapRequestCount)
            mapRequestCount[wtxNew.GetHash()] = 0;

        // Broadcast
        if (!wtxNew.AcceptToMemoryPool())
        {
            // This must not fail. The transaction has already been signed and recorded.
            printf("CommitTransaction() : Error: Transaction not valid");
            return false;
        }
        wtxNew.RelayWalletTransaction();
    }
    MainFrameRepaint();
    return true;
}




// requires cs_main lock
string CWallet::SendMoney(CScript scriptPubKey, int64 nValue, CWalletTx& wtxNew, bool fAskFee)
{
    CReserveKey reservekey(this);
    int64 nFeeRequired;
    if (!CreateTransaction(scriptPubKey, nValue, wtxNew, reservekey, nFeeRequired))
    {
        string strError;
        if (nValue + nFeeRequired > GetBalance())
            strError = strprintf(_("Error: This transaction requires a transaction fee of at least %s because of its amount, complexity, or use of recently received funds  "), FormatMoney(nFeeRequired).c_str());
        else
            strError = _("Error: Transaction creation failed  ");
        printf("SendMoney() : %s", strError.c_str());
        return strError;
    }

    if (fAskFee && !ThreadSafeAskFee(nFeeRequired, _("Sending..."), NULL))
        return "ABORTED";

    if (!CommitTransaction(wtxNew, reservekey))
        return _("Error: The transaction was rejected.  This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");

    MainFrameRepaint();
    return "";
}



// requires cs_main lock
string CWallet::SendMoneyToBitcoinAddress(string strAddress, int64 nValue, CWalletTx& wtxNew, bool fAskFee)
{
    // Check amount
    if (nValue <= 0)
        return _("Invalid amount");
    if (nValue + nTransactionFee > GetBalance())
        return _("Insufficient funds");

    // Parse bitcoin address
    CScript scriptPubKey;
    if (!scriptPubKey.SetBitcoinAddress(strAddress))
        return _("Invalid bitcoin address");

    return SendMoney(scriptPubKey, nValue, wtxNew, fAskFee);
}




bool CWallet::LoadWallet(bool& fFirstRunRet)
{
    if (!fFileBacked)
        return false;
    fFirstRunRet = false;
    if (!CWalletDB(strWalletFile,"cr+").LoadWallet(this))
        return false;
    fFirstRunRet = vchDefaultKey.empty();

    if (!mapKeys.count(vchDefaultKey))
    {
        // Create new default key
        RandAddSeedPerfmon();

        SetDefaultKey(GetKeyFromKeyPool());
        if (!SetAddressBookName(PubKeyToAddress(vchDefaultKey), ""))
            return false;
    }

    CreateThread(ThreadFlushWalletDB, &strWalletFile);
    return true;
}


bool CWallet::SetAddressBookName(const string& strAddress, const string& strName)
{
    mapAddressBook[strAddress] = strName;
    if (!fFileBacked)
        return false;
    return CWalletDB(strWalletFile).WriteName(strAddress, strName);
}

bool CWallet::DelAddressBookName(const string& strAddress)
{
    mapAddressBook.erase(strAddress);
    if (!fFileBacked)
        return false;
    return CWalletDB(strWalletFile).EraseName(strAddress);
}


void CWallet::PrintWallet(const CBlock& block)
{
    CRITICAL_BLOCK(cs_mapWallet)
    {
        if (mapWallet.count(block.vtx[0].GetHash()))
        {
            CWalletTx& wtx = mapWallet[block.vtx[0].GetHash()];
            printf("    mine:  %d  %d  %d", wtx.GetDepthInMainChain(), wtx.GetBlocksToMaturity(), wtx.GetCredit());
        }
    }
    printf("\n");
}

bool CWallet::GetTransaction(const uint256 &hashTx, CWalletTx& wtx)
{
    CRITICAL_BLOCK(cs_mapWallet)
    {
        map<uint256, CWalletTx>::iterator mi = mapWallet.find(hashTx);
        if (mi != mapWallet.end())
        {
            wtx = (*mi).second;
            return true;
        }
    }
    return false;
}

bool CWallet::SetDefaultKey(const std::vector<unsigned char> &vchPubKey)
{
    if (fFileBacked)
    {
        if (!CWalletDB(strWalletFile).WriteDefaultKey(vchPubKey))
            return false;
    }
    vchDefaultKey = vchPubKey;
    return true;
}

bool GetWalletFile(CWallet* pwallet, string &strWalletFileOut)
{
    if (!pwallet->fFileBacked)
        return false;
    strWalletFileOut = pwallet->strWalletFile;
    return true;
}

void CWallet::ReserveKeyFromKeyPool(int64& nIndex, CKeyPool& keypool)
{
    nIndex = -1;
    keypool.vchPubKey.clear();
    CRITICAL_BLOCK(cs_main)
    CRITICAL_BLOCK(cs_mapWallet)
    CRITICAL_BLOCK(cs_setKeyPool)
    {
        CWalletDB walletdb(strWalletFile);

        // Top up key pool
        int64 nTargetSize = max(GetArg("-keypool", 100), (int64)0);
        while (setKeyPool.size() < nTargetSize+1)
        {
            int64 nEnd = 1;
            if (!setKeyPool.empty())
                nEnd = *(--setKeyPool.end()) + 1;
            if (!walletdb.WritePool(nEnd, CKeyPool(GenerateNewKey())))
                throw runtime_error("ReserveKeyFromKeyPool() : writing generated key failed");
            setKeyPool.insert(nEnd);
            printf("keypool added key %"PRI64d", size=%d\n", nEnd, setKeyPool.size());
        }

        // Get the oldest key
        assert(!setKeyPool.empty());
        nIndex = *(setKeyPool.begin());
        setKeyPool.erase(setKeyPool.begin());
        if (!walletdb.ReadPool(nIndex, keypool))
            throw runtime_error("ReserveKeyFromKeyPool() : read failed");
        if (!mapKeys.count(keypool.vchPubKey))
            throw runtime_error("ReserveKeyFromKeyPool() : unknown key in key pool");
        assert(!keypool.vchPubKey.empty());
        printf("keypool reserve %"PRI64d"\n", nIndex);
    }
}

void CWallet::KeepKey(int64 nIndex)
{
    // Remove from key pool
    if (fFileBacked)
    {
        CWalletDB walletdb(strWalletFile);
        CRITICAL_BLOCK(cs_main)
        {
            walletdb.ErasePool(nIndex);
        }
    }
    printf("keypool keep %"PRI64d"\n", nIndex);
}

void CWallet::ReturnKey(int64 nIndex)
{
    // Return to key pool
    CRITICAL_BLOCK(cs_setKeyPool)
        setKeyPool.insert(nIndex);
    printf("keypool return %"PRI64d"\n", nIndex);
}

vector<unsigned char> CWallet::GetKeyFromKeyPool()
{
    int64 nIndex = 0;
    CKeyPool keypool;
    ReserveKeyFromKeyPool(nIndex, keypool);
    KeepKey(nIndex);
    return keypool.vchPubKey;
}

int64 CWallet::GetOldestKeyPoolTime()
{
    int64 nIndex = 0;
    CKeyPool keypool;
    ReserveKeyFromKeyPool(nIndex, keypool);
    ReturnKey(nIndex);
    return keypool.nTime;
}

vector<unsigned char> CReserveKey::GetReservedKey()
{
    if (nIndex == -1)
    {
        CKeyPool keypool;
        pwallet->ReserveKeyFromKeyPool(nIndex, keypool);
        vchPubKey = keypool.vchPubKey;
    }
    assert(!vchPubKey.empty());
    return vchPubKey;
}

void CReserveKey::KeepKey()
{
    if (nIndex != -1)
        pwallet->KeepKey(nIndex);
    nIndex = -1;
    vchPubKey.clear();
}

void CReserveKey::ReturnKey()
{
    if (nIndex != -1)
        pwallet->ReturnKey(nIndex);
    nIndex = -1;
    vchPubKey.clear();
}

