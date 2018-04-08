// Copyright (c) 2011-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/transactionrecord.h>

#include <consensus/consensus.h>
#include <interfaces/wallet.h>
#include <key_io.h>
#include <timedata.h>
#include <validation.h>

#include <stdint.h>

#include <wallet/hdwallet.h>


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

    if (wtx.is_record)
    {
        const CTransactionRecord &rtx = wtx.irtx->second;

        const uint256 &hash = wtx.irtx->first;
        int64_t nTime = rtx.GetTxTime();
        TransactionRecord sub(hash, nTime);

        CTxDestination address = CNoDestination();
        uint8_t nFlags = 0;
        for (auto &r : rtx.vout)
        {
            if (r.nFlags & ORF_CHANGE)
                continue;

            nFlags |= r.nFlags;

            if (r.vPath.size() > 0)
            {
                if (r.vPath[0] == ORA_STEALTH)
                {
                    if (r.vPath.size() < 5)
                    {
                        LogPrintf("%s: Warning, malformed vPath.\n", __func__);
                    } else
                    {
                        uint32_t sidx;
                        memcpy(&sidx, &r.vPath[1], 4);
                        CStealthAddress sx;
                        if (wtx.partWallet->GetStealthByIndex(sidx, sx))
                            address = sx;
                    };
                };
            } else
            {
                if (address.type() == typeid(CNoDestination))
                    ExtractDestination(r.scriptPubKey, address);
            };

            if (r.nType == OUTPUT_STANDARD)
            {
                sub.typeOut = 'P';
            } else
            if (r.nType == OUTPUT_CT)
            {
                sub.typeOut = 'B';
            } else
            if (r.nType == OUTPUT_RINGCT)
            {
                sub.typeOut = 'A';
            };

            if (nFlags & ORF_OWNED)
                sub.credit += r.nValue;
            if (nFlags & ORF_FROM)
                sub.debit -= r.nValue;
        };

        if (address.type() != typeid(CNoDestination))
            sub.address = CBitcoinAddress(address).ToString();


        if (sub.debit != 0)
            sub.debit -= rtx.nFee;

        if (nFlags & ORF_OWNED && nFlags & ORF_FROM)
        {
            sub.type = TransactionRecord::SendToSelf;
        } else
        if (nFlags & ORF_OWNED)
        {
            sub.type = TransactionRecord::RecvWithAddress;

        } else
        if (nFlags & ORF_FROM)
        {
            sub.type = TransactionRecord::SendToAddress;
        };

        if (rtx.nFlags & ORF_ANON_IN)
            sub.typeIn = 'A';
        else
        if (rtx.nFlags & ORF_BLIND_IN)
            sub.typeIn = 'B';

        sub.involvesWatchAddress = nFlags & ORF_OWN_WATCH;
        parts.append(sub);
        return parts;
    }

    if (wtx.is_coinstake)
    {
        int64_t nTime = wtx.time;
        //CAmount nCredit = wtx.credit;
        CAmount nDebit = wtx.debit;
        //CAmount nNet = nCredit - nDebit;
        uint256 hash = wtx.tx->GetHash();

        bool involvesWatchAddress = false;
        TransactionRecord sub(hash, nTime);

        sub.type = TransactionRecord::Staked;
        sub.debit = -nDebit;

        CAmount nCredit = 0;
        for (size_t i = 0; i < wtx.tx->vpout.size(); ++i)
        {
            const CTxOutBase *txout = wtx.tx->vpout[i].get();
            if (!txout->IsType(OUTPUT_STANDARD))
                continue;

            isminetype mine = wtx.txout_is_mine[i];
            if (!mine)
                continue;

            nCredit += txout->GetValue();

            if (sub.address.empty())
            {
                CTxDestination address;
                involvesWatchAddress = involvesWatchAddress || (mine & ISMINE_WATCH_ONLY);
                if (ExtractDestination(*txout->GetPScriptPubKey(), address))
                {
                    sub.address = CBitcoinAddress(address).ToString();
                };
            }
        };

        sub.involvesWatchAddress = involvesWatchAddress;
        sub.credit = nCredit;
        parts.append(sub);

        return parts;
    };

    int64_t nTime = wtx.time;
    CAmount nCredit = wtx.credit;
    CAmount nDebit = wtx.debit;
    CAmount nNet = nCredit - nDebit;
    uint256 hash = wtx.tx->GetHash();
    std::map<std::string, std::string> mapValue = wtx.value_map;

    if (nNet > 0 || wtx.is_coinbase)
    {
        //
        // Credit
        //

        for(unsigned int i = 0; i < wtx.tx->vpout.size(); i++)
        {
            const CTxOutBase *txout = wtx.tx->vpout[i].get();

            if (!txout->IsType(OUTPUT_STANDARD))
                continue;

            isminetype mine = wtx.txout_is_mine[i];
            if (mine)
            {
                TransactionRecord sub(hash, nTime);
                CTxDestination address;
                sub.idx = i; // vout index
                sub.credit = txout->GetValue();
                sub.involvesWatchAddress = mine & ISMINE_WATCH_ONLY;
                if (wtx.txout_address_is_mine[i])
                {
                    // Received by Bitcoin Address
                    if (wtx.tx->IsCoinStake())
                        sub.type = TransactionRecord::Staked;
                    else
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
    } else
    {
        bool involvesWatchAddress = false;
        isminetype fAllFromMe = ISMINE_SPENDABLE;
        for (isminetype mine : wtx.txin_is_mine)
        {
            if(mine & ISMINE_WATCH_ONLY) involvesWatchAddress = true;
            if(fAllFromMe > mine) fAllFromMe = mine;
        }

        isminetype fAllToMe = ISMINE_SPENDABLE;
        for (isminetype mine : wtx.txout_is_mine)
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
        } else
        if (fAllFromMe)
        {
            //
            // Debit
            //
            CAmount nTxFee = nDebit - wtx.tx->GetValueOut();

            for (unsigned int nOut = 0; nOut < wtx.tx->vpout.size(); nOut++)
            {
                const CTxOutBase *txout = wtx.tx->vpout[nOut].get();
                if (!txout->IsType(OUTPUT_STANDARD))
                    continue;

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

                CAmount nValue = txout->GetValue();
                // Add fee to first output
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

void TransactionRecord::updateStatus(const interfaces::WalletTxStatus& wtx, int numBlocks, int64_t adjustedTime)
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

                // Check if the block was requested by anyone
                if (adjustedTime - wtx.time_received > 2 * 60 && wtx.request_count == 0)
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
        else if (adjustedTime - wtx.time_received > 2 * 60 && wtx.request_count == 0)
        {
            status.status = TransactionStatus::Offline;
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
