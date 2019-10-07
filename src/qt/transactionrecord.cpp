// Copyright (c) 2011-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/transactionrecord.h>

#include <base58.h>
#include <consensus/consensus.h>
#include <validation.h>
#include <timedata.h>
#include <wallet/wallet.h>

#include <stdint.h>


/* Return positive answer if transaction should be shown in list.
 */
bool TransactionRecord::showTransaction(const CWalletTx &wtx)
{
    // There are currently no cases where we hide transactions, but
    // we may want to use this in the future for things like RBF.
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

    // Bind plotter, Rental
    auto itType = mapValue.find("type");
    if (itType != mapValue.end())
    {
        if (itType->second == "bindplotter")
        {
            TransactionRecord sub(hash, nTime);
            sub.debit = -wtx.tx->vout[0].nValue + nNet;
            sub.involvesWatchAddress = wallet->IsMine(wtx.tx->vout[0]) & ISMINE_WATCH_ONLY;
            sub.type = TransactionRecord::BindPlotter;
            sub.address = mapValue["from"];
            sub.comment = mapValue["plotter_id"];

            parts.append(sub);
            return parts;
        }
        else if (itType->second == "unbindplotter")
        {
            TransactionRecord sub(hash, nTime);
            sub.credit = wtx.tx->vout[0].nValue;
            sub.involvesWatchAddress = wallet->IsMine(wtx.tx->vout[0]) & ISMINE_WATCH_ONLY;
            sub.type = TransactionRecord::UnbindPlotter;
            sub.address = mapValue["from"];
            sub.comment = mapValue["plotter_id"];

            parts.append(sub);
            return parts;
        }
        else if (itType->second == "pledge")
        {
            isminetype sendIsmine = ::IsMine(*wallet, DecodeDestination(mapValue["from"]));
            isminetype toIsmine = ::IsMine(*wallet, DecodeDestination(mapValue["to"]));
            TransactionRecord sub(hash, nTime);
            if ((sendIsmine & ISMINE_SPENDABLE) && (toIsmine & ISMINE_SPENDABLE)) {
                // Mine -> Mine
                sub.debit = -wtx.tx->vout[0].nValue + nNet;
                sub.involvesWatchAddress = false;
                sub.type = TransactionRecord::SelfRental;
                sub.address = mapValue["to"];
                parts.append(sub);
            } else if (sendIsmine & ISMINE_SPENDABLE) {
                // Mine -> Other
                sub.debit = -wtx.tx->vout[0].nValue + nNet;
                sub.involvesWatchAddress = false;
                sub.type = TransactionRecord::LoanTo;
                sub.address = mapValue["to"];
                parts.append(sub);
            } else if (toIsmine & ISMINE_SPENDABLE) {
                // Other -> Mine
                sub.credit = wtx.tx->vout[0].nValue + nNet;
                sub.involvesWatchAddress = false;
                sub.type = TransactionRecord::BorrowFrom;
                sub.address = mapValue["from"];
                parts.append(sub);
            } else if ((sendIsmine & ISMINE_WATCH_ONLY) && (toIsmine & ISMINE_WATCH_ONLY)) {
                // WatchOnly -> WatchOnly
                sub.debit = -wtx.tx->vout[0].nValue + nNet;
                sub.involvesWatchAddress = true;
                sub.type = TransactionRecord::SelfRental;
                sub.address = mapValue["to"];
                parts.append(sub);
            } else if (sendIsmine & ISMINE_WATCH_ONLY) {
                // WatchOnly -> Other
                sub.debit = -wtx.tx->vout[0].nValue + nNet;
                sub.involvesWatchAddress = true;
                sub.type = TransactionRecord::LoanTo;
                sub.address = mapValue["to"];
                parts.append(sub);
            } else if (toIsmine & ISMINE_WATCH_ONLY) {
                // Other -> WatchOnly
                sub.credit = wtx.tx->vout[0].nValue + nNet;
                sub.involvesWatchAddress = true;
                sub.type = TransactionRecord::BorrowFrom;
                sub.address = mapValue["from"];
                parts.append(sub);
            }

            return parts;
        }
        else if (itType->second == "withdrawpledge")
        {
            isminetype sendIsmine = ::IsMine(*wallet, DecodeDestination(mapValue["from"]));
            isminetype toIsmine = ::IsMine(*wallet, DecodeDestination(mapValue["to"]));
            TransactionRecord sub(hash, nTime);
            sub.credit = wtx.tx->vout[0].nValue;
            sub.involvesWatchAddress = ((sendIsmine & ISMINE_WATCH_ONLY) || (toIsmine & ISMINE_WATCH_ONLY)) &&
                (!(sendIsmine & ISMINE_SPENDABLE) && !(toIsmine & ISMINE_SPENDABLE));
            sub.type = TransactionRecord::WithdrawRental;
            sub.address = sendIsmine ? mapValue["to"] : mapValue["from"];

            parts.append(sub);
            return parts;
        }
    }

    if (nNet > 0 || wtx.IsCoinBase())
    {
        //
        // Credit
        //
        for(unsigned int i = 0; i < wtx.tx->vout.size(); i++)
        {
            const CTxOut& txout = wtx.tx->vout[i];
            isminetype mine = wallet->IsMine(txout);
            if(mine)
            {
                TransactionRecord sub(hash, nTime);
                CTxDestination address;
                sub.idx = i; // vout index
                sub.credit = txout.nValue;
                sub.involvesWatchAddress = mine & ISMINE_WATCH_ONLY;
                if (ExtractDestination(txout.scriptPubKey, address) && IsMine(*wallet, address))
                {
                    // Received by Bitcoin Address
                    sub.type = TransactionRecord::RecvWithAddress;
                    sub.address = EncodeDestination(address);
                }
                else
                {
                    // Received by IP connection (deprecated features), or a multisignature or other non-simple transaction
                    sub.type = TransactionRecord::RecvFromOther;
                    sub.address = mapValue["from"];
                }
                if (wtx.IsCoinBase())
                {
                    // Generated
                    sub.type = TransactionRecord::Generated;
                }

                parts.append(sub);
            }
        }
    }
    else
    {
        bool involvesWatchAddress = false;
        isminetype fAllFromMe = ISMINE_SPENDABLE;
        for (const CTxIn& txin : wtx.tx->vin)
        {
            isminetype mine = wallet->IsMine(txin);
            if(mine & ISMINE_WATCH_ONLY) involvesWatchAddress = true;
            if(fAllFromMe > mine) fAllFromMe = mine;
        }

        isminetype fAllToMe = ISMINE_SPENDABLE;
        for (const CTxOut& txout : wtx.tx->vout)
        {
            if (txout.nValue == 0 && !txout.scriptPubKey.empty() && txout.scriptPubKey[0] == OP_RETURN) continue;

            isminetype mine = wallet->IsMine(txout);
            if(mine & ISMINE_WATCH_ONLY) involvesWatchAddress = true;
            if(fAllToMe > mine) fAllToMe = mine;
        }

        if (fAllFromMe && fAllToMe)
        {
            // Payment to self
            CAmount nChange = wtx.GetChange();

            parts.append(TransactionRecord(hash, nTime, TransactionRecord::SendToSelf, "",
                            -(nDebit - nChange), nCredit - nChange));
            parts.last().involvesWatchAddress = involvesWatchAddress;   // maybe pass to TransactionRecord as constructor argument
        }
        else if (fAllFromMe)
        {
            //
            // Debit
            //
            CAmount nTxFee = nDebit - wtx.tx->GetValueOut();

            for (unsigned int nOut = 0; nOut < wtx.tx->vout.size(); nOut++)
            {
                const CTxOut& txout = wtx.tx->vout[nOut];
                if (txout.nValue == 0 && !txout.scriptPubKey.empty() && txout.scriptPubKey[0] == OP_RETURN)
                {
                    // Ignore OP_RETURN
                    continue;
                }

                TransactionRecord sub(hash, nTime);
                sub.idx = nOut;
                sub.involvesWatchAddress = involvesWatchAddress;

                if(wallet->IsMine(txout))
                {
                    // Ignore parts sent to self, as this is usually the change
                    // from a transaction sent back to our own address.
                    continue;
                }

                CTxDestination address;
                if (ExtractDestination(txout.scriptPubKey, address))
                {
                    // Sent to Bitcoin Address
                    sub.type = TransactionRecord::SendToAddress;
                    sub.address = EncodeDestination(address);
                }
                else
                {
                    // Sent to IP, or other non-address transaction like OP_EVAL
                    sub.type = TransactionRecord::SendToOther;
                    sub.address = mapValue["to"];
                }

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
            parts.append(TransactionRecord(hash, nTime, TransactionRecord::Other, "", nNet, 0));
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
    CBlockIndex* pindex = nullptr;
    BlockMap::iterator mi = mapBlockIndex.find(wtx.hashBlock);
    if (mi != mapBlockIndex.end())
        pindex = (*mi).second;

    // Sort order, unrecorded transactions sort to the top
    status.sortKey = strprintf("%010d-%01d-%010u-%03d",
        (pindex ? pindex->nHeight : std::numeric_limits<int>::max()),
        (wtx.IsCoinBase() ? 1 : 0),
        wtx.nTimeReceived,
        idx);
    status.countsForBalance = wtx.IsTrusted() && !(wtx.GetBlocksToMaturity() > 0);
    status.depth = wtx.GetDepthInMainChain();
    status.cur_num_blocks = chainActive.Height();

    if (!CheckFinalTx(*wtx.tx))
    {
        if (wtx.tx->nLockTime < LOCKTIME_THRESHOLD)
        {
            status.status = TransactionStatus::OpenUntilBlock;
            status.open_for = wtx.tx->nLockTime - chainActive.Height();
        }
        else
        {
            status.status = TransactionStatus::OpenUntilDate;
            status.open_for = wtx.tx->nLockTime;
        }
    }
    // For generated transactions, determine maturity
    else if(type == TransactionRecord::Generated)
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
            if (wtx.isAbandoned())
                status.status = TransactionStatus::Abandoned;
        }
        else if (status.depth < RecommendedNumConfirmations)
        {
            status.status = TransactionStatus::Confirming;
        }
        else
        {
            status.status = TransactionStatus::Confirmed;
        }

        // Rewrite special tx status
        if (status.status == TransactionStatus::Confirming || status.status == TransactionStatus::Confirmed)
        {
            if (type == TransactionRecord::BindPlotter || type == TransactionRecord::LoanTo ||
                type == TransactionRecord::BorrowFrom || type == TransactionRecord::SelfRental)
            {
                COutPoint coinEntry(wtx.tx->GetHash(), 0);
                const Coin &coin = pcoinsTip->AccessCoin(coinEntry);
                if (coin.IsSpent())
                {
                    status.status = (TransactionStatus::Status) (status.status | TransactionStatus::Disabled);
                }
                else if (coin.IsBindPlotter())
                {
                    const CBindPlotterInfo lastBindInfo = pcoinsTip->GetLastBindPlotterInfo(BindPlotterPayload::As(coin.extraData)->GetId());
                    if (lastBindInfo.outpoint != coinEntry)
                        status.status = (TransactionStatus::Status) (status.status | TransactionStatus::Inactived);
                }
            }
        }
    }
    status.needsUpdate = false;
}

bool TransactionRecord::statusUpdateNeeded() const
{
    AssertLockHeld(cs_main);
    return status.cur_num_blocks != chainActive.Height() || status.needsUpdate;
}

QString TransactionRecord::getTxID() const
{
    return QString::fromStdString(hash.ToString());
}

int TransactionRecord::getOutputIndex() const
{
    return idx;
}
