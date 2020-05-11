// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/transactionrecord.h>

#include <chain.h>
#include <interfaces/wallet.h>
#include <key_io.h>
#include <wallet/ismine.h>

#include <stdint.h>

#include <QDateTime>

/* Return positive answer if transaction should be shown in list.
 */
bool TransactionRecord::showTransaction()
{
    // There are currently no cases where we hide transactions, but
    // we may want to use this in the future for things like RBF.
    return true;
}

/*
 * Decompose CWallet transaction to model transaction records.
 */
QList<TransactionRecord> TransactionRecord::decomposeTransaction(const interfaces::WalletTx& wtx)
{
    QList<TransactionRecord> parts;
    int64_t nTime = wtx.time;
    CAmount nCredit = wtx.credit;
    CAmount nDebit = wtx.debit;
    CAmount nNet = nCredit - nDebit;
    uint256 hash = wtx.tx->GetHash();
    std::map<std::string, std::string> mapValue = wtx.value_map;

    // Bind plotter and point
    auto itType = mapValue.find("type");
    if (itType != mapValue.end())
    {
        if (itType->second == "bindplotter")
        {
            TransactionRecord sub(hash, nTime);
            sub.debit = -wtx.tx->vout[0].nValue + nNet;
            sub.involvesWatchAddress = wtx.txout_is_mine[0] & ISMINE_WATCH_ONLY;
            sub.type = TransactionRecord::BindPlotter;
            sub.address = EncodeDestination(wtx.txout_address[0]);
            sub.comment = mapValue["plotter_id"];

            parts.append(sub);
            return parts;
        }
        else if (itType->second == "unbindplotter")
        {
            TransactionRecord sub(hash, nTime);
            sub.credit = wtx.tx->vout[0].nValue;
            sub.involvesWatchAddress = wtx.txout_is_mine[0] & ISMINE_WATCH_ONLY;
            sub.type = TransactionRecord::UnbindPlotter;
            sub.address = EncodeDestination(wtx.txout_address[0]);
            sub.comment = mapValue["plotter_id"];

            parts.append(sub);
            return parts;
        }
        else if (itType->second == "pledge")
        {
            isminetype senderIsmine = wtx.txout_is_mine[0];
            isminetype receiverIsmine = wtx.tx_point_address_is_mine;
            TransactionRecord sub(hash, nTime);
            if ((senderIsmine & ISMINE_SPENDABLE) && (receiverIsmine & ISMINE_SPENDABLE)) {
                // Mine -> Mine
                sub.debit = -wtx.tx->vout[0].nValue + nNet;
                sub.involvesWatchAddress = false;
                sub.type = TransactionRecord::SelfPoint;
                sub.address = EncodeDestination(wtx.tx_point_address);
                parts.append(sub);
            } else if (senderIsmine & ISMINE_SPENDABLE) {
                // Mine -> Other
                sub.debit = -wtx.tx->vout[0].nValue + nNet;
                sub.involvesWatchAddress = false;
                sub.type = TransactionRecord::PointSent;
                sub.address = EncodeDestination(wtx.tx_point_address);
                parts.append(sub);
            } else if (receiverIsmine & ISMINE_SPENDABLE) {
                // Other -> Mine
                sub.credit = wtx.tx->vout[0].nValue + nNet;
                sub.involvesWatchAddress = false;
                sub.type = TransactionRecord::PointReceived;
                sub.address = EncodeDestination(wtx.txout_address[0]);
                parts.append(sub);
            } else if ((senderIsmine & ISMINE_WATCH_ONLY) && (receiverIsmine & ISMINE_WATCH_ONLY)) {
                // WatchOnly -> WatchOnly
                sub.debit = -wtx.tx->vout[0].nValue + nNet;
                sub.involvesWatchAddress = true;
                sub.type = TransactionRecord::SelfPoint;
                sub.address = EncodeDestination(wtx.tx_point_address);
                parts.append(sub);
            } else if (senderIsmine & ISMINE_WATCH_ONLY) {
                // WatchOnly -> Other
                sub.debit = -wtx.tx->vout[0].nValue + nNet;
                sub.involvesWatchAddress = true;
                sub.type = TransactionRecord::PointSent;
                sub.address = EncodeDestination(wtx.tx_point_address);
                parts.append(sub);
            } else if (receiverIsmine & ISMINE_WATCH_ONLY) {
                // Other -> WatchOnly
                sub.credit = wtx.tx->vout[0].nValue + nNet;
                sub.involvesWatchAddress = true;
                sub.type = TransactionRecord::PointReceived;
                sub.address = EncodeDestination(wtx.txout_address[0]);
                parts.append(sub);
            }

            return parts;
        }
        else if (itType->second == "withdrawpledge")
        {
            isminetype senderIsmine = wtx.txout_is_mine[0];
            isminetype receiverIsmine = wtx.tx_point_address_is_mine;
            TransactionRecord sub(hash, nTime);
            sub.credit = wtx.tx->vout[0].nValue;
            sub.involvesWatchAddress = ((senderIsmine & ISMINE_WATCH_ONLY) || (receiverIsmine & ISMINE_WATCH_ONLY)) &&
                (!(senderIsmine & ISMINE_SPENDABLE) && !(receiverIsmine & ISMINE_SPENDABLE));
            sub.type = TransactionRecord::WithdrawPoint;
            sub.address = senderIsmine ? EncodeDestination(wtx.txout_address[0]) : EncodeDestination(wtx.tx_point_address);

            parts.append(sub);
            return parts;
        }
    }

    if (nNet > 0 || wtx.is_coinbase)
    {
        //
        // Credit
        //
        for(unsigned int i = 0; i < wtx.tx->vout.size(); i++)
        {
            const CTxOut& txout = wtx.tx->vout[i];
            isminetype mine = wtx.txout_is_mine[i];
            if(mine)
            {
                TransactionRecord sub(hash, nTime);
                CTxDestination address;
                sub.idx = i; // vout index
                sub.credit = txout.nValue;
                sub.involvesWatchAddress = mine & ISMINE_WATCH_ONLY;
                if (wtx.txout_address_is_mine[i])
                {
                    // Received by Bitcoin Address
                    sub.type = TransactionRecord::RecvWithAddress;
                    sub.address = EncodeDestination(wtx.txout_address[i]);
                }
                else
                {
                    // Received by IP connection (deprecated features), or a multisignature or other non-simple transaction
                    sub.type = TransactionRecord::RecvFromOther;
                    sub.address = mapValue["from"];
                }
                if (wtx.is_coinbase)
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
        for (const isminetype mine : wtx.txin_is_mine)
        {
            if(mine & ISMINE_WATCH_ONLY) involvesWatchAddress = true;
            if(fAllFromMe > mine) fAllFromMe = mine;
        }

        isminetype fAllToMe = ISMINE_SPENDABLE;
        for (const isminetype mine : wtx.txout_is_mine)
        {
            if(mine & ISMINE_WATCH_ONLY) involvesWatchAddress = true;
            if(fAllToMe > mine) fAllToMe = mine;
        }

        if (fAllFromMe && fAllToMe)
        {
            // Payment to self
            CAmount nChange = wtx.change;

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
                TransactionRecord sub(hash, nTime);
                sub.idx = nOut;
                sub.involvesWatchAddress = involvesWatchAddress;

                if(wtx.txout_is_mine[nOut])
                {
                    // Ignore parts sent to self, as this is usually the change
                    // from a transaction sent back to our own address.
                    continue;
                }

                if (!boost::get<CNoDestination>(&wtx.txout_address[nOut]))
                {
                    // Sent to Bitcoin Address
                    sub.type = TransactionRecord::SendToAddress;
                    sub.address = EncodeDestination(wtx.txout_address[nOut]);
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

void TransactionRecord::updateStatus(const interfaces::WalletTxStatus& wtx, int numBlocks, int64_t block_time)
{
    // Determine transaction status

    // Sort order, unrecorded transactions sort to the top
    status.sortKey = strprintf("%010d-%01d-%010u-%03d",
        wtx.block_height,
        wtx.is_coinbase ? 1 : 0,
        wtx.time_received,
        idx);
    status.countsForBalance = wtx.is_trusted && !(wtx.blocks_to_maturity > 0);
    status.depth = wtx.depth_in_main_chain;
    status.cur_num_blocks = numBlocks;

    const bool up_to_date = ((int64_t)QDateTime::currentMSecsSinceEpoch() / 1000 - block_time < MAX_BLOCK_TIME_GAP);
    if (up_to_date && !wtx.is_final) {
        if (wtx.lock_time < LOCKTIME_THRESHOLD) {
            status.status = TransactionStatus::OpenUntilBlock;
            status.open_for = wtx.lock_time - numBlocks;
        }
        else
        {
            status.status = TransactionStatus::OpenUntilDate;
            status.open_for = wtx.lock_time;
        }
    }
    // For generated transactions, determine maturity
    else if(type == TransactionRecord::Generated)
    {
        if (wtx.blocks_to_maturity > 0)
        {
            status.status = TransactionStatus::Immature;

            if (wtx.is_in_main_chain)
            {
                status.matures_in = wtx.blocks_to_maturity;
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
        else if (status.depth == 0)
        {
            status.status = TransactionStatus::Unconfirmed;
            if (wtx.is_abandoned)
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
    }
    // Rewrite special tx status
    if (status.status == TransactionStatus::Confirming || status.status == TransactionStatus::Confirmed)
    {
        if (type == TransactionRecord::BindPlotter || type == TransactionRecord::PointSent ||
            type == TransactionRecord::PointReceived || type == TransactionRecord::SelfPoint)
        {
            if (wtx.is_unfrozen)
            {
                status.status = (TransactionStatus::Status) (status.status | TransactionStatus::Disabled);
            }
            else if (wtx.is_bindplotter_inactived)
            {
                status.status = (TransactionStatus::Status) (status.status | TransactionStatus::Inactived);
            }
        }
    }
    status.needsUpdate = false;
}

bool TransactionRecord::statusUpdateNeeded(int numBlocks) const
{
    return status.cur_num_blocks != numBlocks || status.needsUpdate;
}

QString TransactionRecord::getTxHash() const
{
    return QString::fromStdString(hash.ToString());
}

int TransactionRecord::getOutputIndex() const
{
    return idx;
}
