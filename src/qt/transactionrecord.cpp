// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "transactionrecord.h"

#include "base58.h"
#include "consensus/consensus.h"
#include "main.h"
#include "timedata.h"

#include <stdint.h>

#include <boost/foreach.hpp>

/* Return positive answer if transaction should be shown in list.
 */
bool TransactionRecord::showTransaction(const CWalletTx &wtx)
{
    if (wtx.IsCoinBase())
    {
        // Ensures we show generated coins / mined transactions at depth 1
        if (!wtx.IsInMainChain())
        {
            return false;
        }
    }
    return true;
}

/*
 * Decompose CWallet transaction to model transaction records.
 */
QList<TransactionRecord> TransactionRecord::decomposeTransaction(const CWallet *wallet, const CWalletTx &wtx)
{
    QList<TransactionRecord> parts;
    int64_t nTime = wtx.GetTxTime();
    CAmount nCredit = wtx.GetCredit(ISMINE_ALL);
    CAmount nDebit = wtx.GetDebit(ISMINE_ALL);
    CAmount nNet = nCredit - nDebit;
    uint256 hash = wtx.GetHash();
    std::map<std::string, std::string> mapValue = wtx.mapValue;

    if (nNet > 0 || wtx.IsCoinBase())
    {
        //
        // Credit
        //
        std::string labelPublic = "";
        BOOST_FOREACH (const CTxOut &txout, wtx.vout)
        {
            isminetype mine = wallet->IsMine(txout);
            if (mine)
            {
                TransactionRecord sub(hash, nTime);
                CTxDestination address;
                sub.idx = parts.size(); // sequence number
                sub.credit = txout.nValue;
                sub.involvesWatchAddress = mine & ISMINE_WATCH_ONLY;

                // the public label refers to the following utxo
                if (labelPublic == "")
                {
                    labelPublic = getLabelPublic(txout.scriptPubKey);
                    if (labelPublic != "")
                        continue;
                }

                if (wtx.IsCoinBase())
                {
                    // Generated
                    sub.type = TransactionRecord::Generated;
                }
                else if (ExtractDestination(txout.scriptPubKey, address) && wallet->IsMine(address))
                {
                    // Received by Bitcoin Address
                    sub.type = TransactionRecord::RecvWithAddress;
                    if (labelPublic == "")
                        sub.addresses.push_back(
                            std::make_pair(CBitcoinAddress(address).ToString(), txout.scriptPubKey));
                    else
                        sub.addresses.push_back(std::make_pair(
                            "<" + labelPublic + "> " + CBitcoinAddress(address).ToString(), txout.scriptPubKey));
                }
                else
                {
                    // Received by IP connection (deprecated features), or a multisignature or other non-simple
                    // transaction
                    sub.type = TransactionRecord::RecvFromOther;
                    sub.addresses.push_back(std::make_pair(mapValue["from"], txout.scriptPubKey));
                }

                parts.append(sub);
            }

            labelPublic = "";
        }
    }
    else
    {
        bool involvesWatchAddress = false;
        isminetype fAllToMe = ISMINE_SPENDABLE;
        BOOST_FOREACH (const CTxOut &txout, wtx.vout)
        {
            isminetype mine = wallet->IsMine(txout);
            if (mine & ISMINE_WATCH_ONLY)
                involvesWatchAddress = true;
            if (fAllToMe > mine)
                fAllToMe = mine;
        }

        // load all tx addresses for user display/filter
        AddressList listAllAddresses;
        CTxDestination address;
        BOOST_FOREACH (const CTxOut &txout, wtx.vout)
        {
            if (!fAllToMe && wallet->IsMine(txout))
            {
                continue; // skip change if some tx went to somewhere else
            }
            // get public label if it exists
            std::string labelPublic = getLabelPublic(txout.scriptPubKey);
            if (labelPublic != "")
                // use public label before address
                listAllAddresses.push_back(std::make_pair("<" + labelPublic + ">", txout.scriptPubKey));
            else if (ExtractDestination(txout.scriptPubKey, address))
                // a standard address
                listAllAddresses.push_back(std::make_pair(CBitcoinAddress(address).ToString(), txout.scriptPubKey));

            else
                // add the unknown scriptPubKey as n/a - TODO could also skip these if there is no need to
                // display/filter??
                listAllAddresses.push_back(std::make_pair("n/a", txout.scriptPubKey));
        }

        isminetype fAllFromMe = ISMINE_SPENDABLE;
        BOOST_FOREACH (const CTxIn &txin, wtx.vin)
        {
            isminetype mine = wallet->IsMine(txin);
            if (mine & ISMINE_WATCH_ONLY)
                involvesWatchAddress = true;
            if (fAllFromMe > mine)
                fAllFromMe = mine;
        }

        if (fAllFromMe && fAllToMe)
        {
            // Payment to self
            CAmount nChange = wtx.GetChange();
            parts.append(TransactionRecord(
                hash, nTime, TransactionRecord::SendToSelf, listAllAddresses, -(nDebit - nChange), nCredit - nChange));

            // maybe pass to TransactionRecord as constructor argument
            parts.last().involvesWatchAddress = involvesWatchAddress;
        }
        else if (fAllFromMe)
        {
            //
            // Debit
            //
            CAmount nTxFee = nDebit - wtx.GetValueOut();

            for (unsigned int nOut = 0; nOut < wtx.vout.size(); nOut++)
            {
                const CTxOut &txout = wtx.vout[nOut];

                if (wallet->IsMine(txout))
                {
                    // Ignore parts sent to self, as this is usually the change
                    // from a transaction sent back to our own address.
                    continue;
                }

                TransactionRecord sub(hash, nTime);
                sub.idx = parts.size();
                sub.involvesWatchAddress = involvesWatchAddress;

                CTxDestination address;
                std::string labelPublic = getLabelPublic(txout.scriptPubKey);
                if (labelPublic != "")
                    continue;
                else if (ExtractDestination(txout.scriptPubKey, address))
                {
                    // Sent to Bitcoin Address
                    sub.type = TransactionRecord::SendToAddress;
                    // sub.addresses.push_back(std::make_pair(CBitcoinAddress(address).ToString(), txout.scriptPubKey));
                }
                else
                {
                    // Sent to IP, or other non-address transaction like OP_EVAL
                    sub.type = TransactionRecord::SendToOther;
                    // sub.addresses.push_back(std::make_pair(mapValue["to"], txout.scriptPubKey));
                }

                sub.addresses = listAllAddresses;

                CAmount nValue = txout.nValue;
                /* Add fee to first output */
                if (nTxFee > 0)
                {
                    nValue += nTxFee;
                    nTxFee = 0;
                }
                sub.debit = -nValue;

                parts.append(sub);
            }
        }
        else
        {
            //
            // Mixed debit transaction, can't break down payees
            //
            parts.append(TransactionRecord(hash, nTime, TransactionRecord::Other, listAllAddresses, nNet, 0));
            parts.last().involvesWatchAddress = involvesWatchAddress;
        }
    }

    return parts;
}

void TransactionRecord::updateStatus(const CWalletTx &wtx)
{
    AssertLockHeld(cs_main);
    // Determine transaction status

    // Find the block the tx is in
    CBlockIndex *pindex = NULL;
    BlockMap::iterator mi = mapBlockIndex.find(wtx.hashBlock);
    if (mi != mapBlockIndex.end())
        pindex = (*mi).second;

    // Sort order, unrecorded transactions sort to the top
    status.sortKey = strprintf("%010d-%01d-%010u-%03d", (pindex ? pindex->nHeight : std::numeric_limits<int>::max()),
        (wtx.IsCoinBase() ? 1 : 0), wtx.nTimeReceived, idx);
    status.countsForBalance = wtx.IsTrusted() && !(wtx.GetBlocksToMaturity() > 0);
    status.depth = wtx.GetDepthInMainChain();
    status.cur_num_blocks = chainActive.Height();

    if (!CheckFinalTx(wtx))
    {
        if (wtx.nLockTime < LOCKTIME_THRESHOLD)
        {
            status.status = TransactionStatus::OpenUntilBlock;
            status.open_for = wtx.nLockTime - chainActive.Height();
        }
        else
        {
            status.status = TransactionStatus::OpenUntilDate;
            status.open_for = wtx.nLockTime;
        }
    }
    // For generated transactions, determine maturity
    else if (type == TransactionRecord::Generated)
    {
        if (wtx.GetBlocksToMaturity() > 0)
        {
            status.status = TransactionStatus::Immature;

            if (wtx.IsInMainChain())
            {
                status.matures_in = wtx.GetBlocksToMaturity();

                // Check if the block was requested by anyone
                if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
                    status.status = TransactionStatus::MaturesWarning;
            }
            else
            {
                status.status = TransactionStatus::NotAccepted;
            }
        }
        else
        {
            status.status = TransactionStatus::Confirmed;
        }
    }
    else
    {
        if (status.depth < 0)
        {
            status.status = TransactionStatus::Conflicted;
        }
        else if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
        {
            status.status = TransactionStatus::Offline;
        }
        else if (status.depth == 0)
        {
            status.status = TransactionStatus::Unconfirmed;
        }
        else if (status.depth < RecommendedNumConfirmations)
        {
            status.status = TransactionStatus::Confirming;
        }
        else
        {
            status.status = TransactionStatus::Confirmed;
        }
    }
}

bool TransactionRecord::statusUpdateNeeded()
{
    AssertLockHeld(cs_main);
    return status.cur_num_blocks != chainActive.Height();
}

QString TransactionRecord::getTxID() const { return formatSubTxId(hash, idx); }
QString TransactionRecord::formatSubTxId(const uint256 &hash, int vout)
{
    return QString::fromStdString(hash.ToString() + strprintf("-%03d", vout));
}
