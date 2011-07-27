#include "transactionrecord.h"

#include "headers.h"

/* Return positive answer if transaction should be shown in list.
 */
bool TransactionRecord::showTransaction(const CWalletTx &wtx)
{
    if (wtx.IsCoinBase())
    {
        // Don't show generated coin until confirmed by at least one block after it
        // so we don't get the user's hopes up until it looks like it's probably accepted.
        //
        // It is not an error when generated blocks are not accepted.  By design,
        // some percentage of blocks, like 10% or more, will end up not accepted.
        // This is the normal mechanism by which the network copes with latency.
        //
        // We display regular transactions right away before any confirmation
        // because they can always get into some block eventually.  Generated coins
        // are special because if their block is not accepted, they are not valid.
        //
        if (wtx.GetDepthInMainChain() < 2)
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
    int64 nTime = wtx.nTimeDisplayed = wtx.GetTxTime();
    int64 nCredit = wtx.GetCredit(true);
    int64 nDebit = wtx.GetDebit();
    int64 nNet = nCredit - nDebit;
    uint256 hash = wtx.GetHash();
    std::map<std::string, std::string> mapValue = wtx.mapValue;

    if (showTransaction(wtx))
    {
        if (nNet > 0 || wtx.IsCoinBase())
        {
            //
            // Credit
            //
            TransactionRecord sub(hash, nTime);

            sub.credit = nNet;

            if (wtx.IsCoinBase())
            {
                // Generated
                sub.type = TransactionRecord::Generated;

                if (nCredit == 0)
                {
                    int64 nUnmatured = 0;
                    BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                        nUnmatured += wallet->GetCredit(txout);
                    sub.credit = nUnmatured;
                }
            }
            else if (!mapValue["from"].empty() || !mapValue["message"].empty())
            {
                // Received by IP connection
                sub.type = TransactionRecord::RecvFromIP;
                if (!mapValue["from"].empty())
                    sub.address = mapValue["from"];
            }
            else
            {
                // Received by Bitcoin Address
                sub.type = TransactionRecord::RecvWithAddress;
                BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                {
                    if(wallet->IsMine(txout))
                    {
                        CBitcoinAddress address;
                        if (ExtractAddress(txout.scriptPubKey, wallet, address))
                        {
                            sub.address = address.ToString();
                        }
                        break;
                    }
                }
            }
            parts.append(sub);
        }
        else
        {
            bool fAllFromMe = true;
            BOOST_FOREACH(const CTxIn& txin, wtx.vin)
                fAllFromMe = fAllFromMe && wallet->IsMine(txin);

            bool fAllToMe = true;
            BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                fAllToMe = fAllToMe && wallet->IsMine(txout);

            if (fAllFromMe && fAllToMe)
            {
                // Payment to self
                int64 nChange = wtx.GetChange();

                parts.append(TransactionRecord(hash, nTime, TransactionRecord::SendToSelf, "",
                                -(nDebit - nChange), nCredit - nChange));
            }
            else if (fAllFromMe)
            {
                //
                // Debit
                //
                int64 nTxFee = nDebit - wtx.GetValueOut();

                for (int nOut = 0; nOut < wtx.vout.size(); nOut++)
                {
                    const CTxOut& txout = wtx.vout[nOut];
                    TransactionRecord sub(hash, nTime);
                    sub.idx = parts.size();

                    if(wallet->IsMine(txout))
                    {
                        // Ignore parts sent to self, as this is usually the change
                        // from a transaction sent back to our own address.
                        continue;
                    }
                    else if(!mapValue["to"].empty())
                    {
                        // Sent to IP
                        sub.type = TransactionRecord::SendToIP;
                        sub.address = mapValue["to"];
                    }
                    else
                    {
                        // Sent to Bitcoin Address
                        sub.type = TransactionRecord::SendToAddress;
                        CBitcoinAddress address;
                        if (ExtractAddress(txout.scriptPubKey, 0, address))
                        {
                            sub.address = address.ToString();
                        }
                    }

                    int64 nValue = txout.nValue;
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
                bool fAllMine = true;
                BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                    fAllMine = fAllMine && wallet->IsMine(txout);
                BOOST_FOREACH(const CTxIn& txin, wtx.vin)
                    fAllMine = fAllMine && wallet->IsMine(txin);

                parts.append(TransactionRecord(hash, nTime, TransactionRecord::Other, "", nNet, 0));
            }
        }
    }

    return parts;
}

void TransactionRecord::updateStatus(const CWalletTx &wtx)
{
    // Determine transaction status

    // Find the block the tx is in
    CBlockIndex* pindex = NULL;
    std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(wtx.hashBlock);
    if (mi != mapBlockIndex.end())
        pindex = (*mi).second;

    // Sort order, unrecorded transactions sort to the top
    status.sortKey = strprintf("%010d-%01d-%010u-%03d",
        (pindex ? pindex->nHeight : INT_MAX),
        (wtx.IsCoinBase() ? 1 : 0),
        wtx.nTimeReceived,
        idx);
    status.confirmed = wtx.IsConfirmed();
    status.depth = wtx.GetDepthInMainChain();
    status.cur_num_blocks = nBestHeight;

    if (!wtx.IsFinal())
    {
        if (wtx.nLockTime < LOCKTIME_THRESHOLD)
        {
            status.status = TransactionStatus::OpenUntilBlock;
            status.open_for = nBestHeight - wtx.nLockTime;
        }
        else
        {
            status.status = TransactionStatus::OpenUntilDate;
            status.open_for = wtx.nLockTime;
        }
    }
    else
    {
        if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
        {
            status.status = TransactionStatus::Offline;
        }
        else if (status.depth < NumConfirmations)
        {
            status.status = TransactionStatus::Unconfirmed;
        }
        else
        {
            status.status = TransactionStatus::HaveConfirmations;
        }
    }

    // For generated transactions, determine maturity
    if(type == TransactionRecord::Generated)
    {
        int64 nCredit = wtx.GetCredit(true);
        if (nCredit == 0)
        {
            status.maturity = TransactionStatus::Immature;

            if (wtx.IsInMainChain())
            {
                status.matures_in = wtx.GetBlocksToMaturity();

                // Check if the block was requested by anyone
                if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
                    status.maturity = TransactionStatus::MaturesWarning;
            }
            else
            {
                status.maturity = TransactionStatus::NotAccepted;
            }
        }
        else
        {
            status.maturity = TransactionStatus::Mature;
        }
    }
}

bool TransactionRecord::statusUpdateNeeded()
{
    return status.cur_num_blocks != nBestHeight;
}

std::string TransactionRecord::getTxID()
{
    return hash.ToString() + strprintf("-%03d", idx);
}

