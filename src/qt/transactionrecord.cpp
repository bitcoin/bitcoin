// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2014-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/transactionrecord.h>

#include <consensus/consensus.h>
#include <interfaces/wallet.h>
#include <interfaces/node.h>
#include <timedata.h>
#include <validation.h>

#include <stdint.h>


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
QList<TransactionRecord> TransactionRecord::decomposeTransaction(interfaces::Wallet& wallet, const interfaces::WalletTx& wtx)
{
    QList<TransactionRecord> parts;
    int64_t nTime = wtx.time;
    CAmount nCredit = wtx.credit;
    CAmount nDebit = wtx.debit;
    CAmount nNet = nCredit - nDebit;
    uint256 hash = wtx.tx->GetHash();
    std::map<std::string, std::string> mapValue = wtx.value_map;
    auto node = interfaces::MakeNode();
    auto& coinJoinOptions = node->coinJoinOptions();

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
                sub.idx = i; // vout index
                sub.credit = txout.nValue;
                sub.involvesWatchAddress = mine & ISMINE_WATCH_ONLY;
                if (wtx.txout_address_is_mine[i])
                {
                    // Received by Dash Address
                    sub.type = TransactionRecord::RecvWithAddress;
                    sub.strAddress = EncodeDestination(wtx.txout_address[i]);
                    sub.txDest = wtx.txout_address[i];
                    sub.updateLabel(wallet);
                }
                else
                {
                    // Received by IP connection (deprecated features), or a multisignature or other non-simple transaction
                    sub.type = TransactionRecord::RecvFromOther;
                    sub.strAddress = mapValue["from"];
                    sub.txDest = DecodeDestination(sub.strAddress);
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

        if(wtx.is_denominate) {
            parts.append(TransactionRecord(hash, nTime, TransactionRecord::CoinJoinMixing, "", -nDebit, nCredit));
            parts.last().involvesWatchAddress = false;   // maybe pass to TransactionRecord as constructor argument
        }
        else if (fAllFromMe && fAllToMe)
        {
            // Payment to self
            // TODO: this section still not accurate but covers most cases,
            // might need some additional work however

            TransactionRecord sub(hash, nTime);
            // Payment to self by default
            sub.type = TransactionRecord::SendToSelf;
            sub.strAddress = "";

            if(mapValue["DS"] == "1")
            {
                sub.type = TransactionRecord::CoinJoinSend;
                CTxDestination address;
                if (ExtractDestination(wtx.tx->vout[0].scriptPubKey, address))
                {
                    // Sent to Dash Address
                    sub.strAddress = EncodeDestination(address);
                    sub.txDest = address;
                    sub.updateLabel(wallet);
                }
                else
                {
                    // Sent to IP, or other non-address transaction like OP_EVAL
                    sub.strAddress = mapValue["to"];
                    sub.txDest = DecodeDestination(sub.strAddress);
                }
            }
            else
            {
                sub.idx = parts.size();
                if(wtx.tx->vin.size() == 1 && wtx.tx->vout.size() == 1
                    && coinJoinOptions.isCollateralAmount(nDebit)
                    && coinJoinOptions.isCollateralAmount(nCredit)
                    && coinJoinOptions.isCollateralAmount(-nNet))
                {
                    sub.type = TransactionRecord::CoinJoinCollateralPayment;
                } else {
                    bool fMakeCollateral{false};
                    if (wtx.tx->vout.size() == 2) {
                        CAmount nAmount0 = wtx.tx->vout[0].nValue;
                        CAmount nAmount1 = wtx.tx->vout[1].nValue;
                        // <case1>, see CCoinJoinClientSession::MakeCollateralAmounts
                        fMakeCollateral = (nAmount0 == coinJoinOptions.getMaxCollateralAmount() && !coinJoinOptions.isDenominated(nAmount1) && nAmount1 >= coinJoinOptions.getMinCollateralAmount()) ||
                                          (nAmount1 == coinJoinOptions.getMaxCollateralAmount() && !coinJoinOptions.isDenominated(nAmount0) && nAmount0 >= coinJoinOptions.getMinCollateralAmount()) ||
                        // <case2>, see CCoinJoinClientSession::MakeCollateralAmounts
                                          (nAmount0 == nAmount1 && coinJoinOptions.isCollateralAmount(nAmount0));
                    } else if (wtx.tx->vout.size() == 1) {
                        // <case3>, see CCoinJoinClientSession::MakeCollateralAmounts
                        fMakeCollateral = coinJoinOptions.isCollateralAmount(wtx.tx->vout[0].nValue);
                    }
                    if (fMakeCollateral) {
                        sub.type = TransactionRecord::CoinJoinMakeCollaterals;
                    } else {
                        for (const auto& txout : wtx.tx->vout) {
                            if (coinJoinOptions.isDenominated(txout.nValue)) {
                                sub.type = TransactionRecord::CoinJoinCreateDenominations;
                                break; // Done, it's definitely a tx creating mixing denoms, no need to look any further
                            }
                        }
                    }
                }
            }

            CAmount nChange = wtx.change;

            sub.debit = -(nDebit - nChange);
            sub.credit = nCredit - nChange;
            parts.append(sub);
            parts.last().involvesWatchAddress = involvesWatchAddress;   // maybe pass to TransactionRecord as constructor argument
        }
        else if (fAllFromMe)
        {
            //
            // Debit
            //
            CAmount nTxFee = nDebit - wtx.tx->GetValueOut();

            bool fDone = false;
            if(wtx.tx->vin.size() == 1 && wtx.tx->vout.size() == 1
                && coinJoinOptions.isCollateralAmount(nDebit)
                && nCredit == 0 // OP_RETURN
                && coinJoinOptions.isCollateralAmount(-nNet))
            {
                TransactionRecord sub(hash, nTime);
                sub.idx = 0;
                sub.type = TransactionRecord::CoinJoinCollateralPayment;
                sub.debit = -nDebit;
                parts.append(sub);
                fDone = true;
            }

            for (unsigned int nOut = 0; nOut < wtx.tx->vout.size() && !fDone; nOut++)
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
                    // Sent to Dash Address
                    sub.type = TransactionRecord::SendToAddress;
                    sub.strAddress = EncodeDestination(wtx.txout_address[nOut]);
                    sub.txDest = wtx.txout_address[nOut];
                    sub.updateLabel(wallet);
                }
                else
                {
                    // Sent to IP, or other non-address transaction like OP_EVAL
                    sub.type = TransactionRecord::SendToOther;
                    sub.strAddress = mapValue["to"];
                    sub.txDest = DecodeDestination(sub.strAddress);
                }

                if(mapValue["DS"] == "1")
                {
                    sub.type = TransactionRecord::CoinJoinSend;
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

void TransactionRecord::updateStatus(const interfaces::WalletTxStatus& wtx, int numBlocks, int chainLockHeight)
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
    status.cachedChainLockHeight = chainLockHeight;
    status.lockedByChainLocks = wtx.is_chainlocked;
    status.lockedByInstantSend = wtx.is_islocked;

    if (!wtx.is_final)
    {
        if (wtx.lock_time < LOCKTIME_THRESHOLD)
        {
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
        else if (status.depth < RecommendedNumConfirmations && !status.lockedByChainLocks)
        {
            status.status = TransactionStatus::Confirming;
        }
        else
        {
            status.status = TransactionStatus::Confirmed;
        }
    }
    status.needsUpdate = false;
}

bool TransactionRecord::statusUpdateNeeded(int numBlocks, int chainLockHeight) const
{
    return status.cur_num_blocks != numBlocks || status.needsUpdate
        || (!status.lockedByChainLocks && status.cachedChainLockHeight != chainLockHeight);
}

void TransactionRecord::updateLabel(interfaces::Wallet& wallet)
{
    if (IsValidDestination(txDest)) {
        std::string name;
        if (wallet.getAddress(txDest, &name, /* is_mine= */ nullptr, /* purpose= */ nullptr)) {
            label = QString::fromStdString(name);
        } else {
            label = "";
        }
    }
}

QString TransactionRecord::getTxHash() const
{
    return QString::fromStdString(hash.ToString());
}

int TransactionRecord::getOutputIndex() const
{
    return idx;
}
