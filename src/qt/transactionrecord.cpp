// Copyright (c) 2011-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/transactionrecord.h>

#include <base58.h>
#include <consensus/consensus.h>
#include <validation.h>
#include <timedata.h>
#include <wallet/wallet.h>
#include <wallet/hdwallet.h>

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

    CAmount nDebit = wtx.GetDebit(ISMINE_ALL);
    uint256 hash = wtx.GetHash();
    std::map<std::string, std::string> mapValue = wtx.mapValue;

    CHDWallet *phdw = (CHDWallet*) wallet;

    if (wtx.IsCoinStake())
    {
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

            isminetype mine = phdw->IsMine(txout);
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

    CAmount nCredit = wtx.GetCredit(ISMINE_ALL);
    CAmount nNet = nCredit - nDebit;

    if (nNet > 0 || wtx.IsCoinBase())
    {
        //
        // Credit
        //

        for(unsigned int i = 0; i < wtx.tx->vpout.size(); i++)
        {
            const CTxOutBase *txout = wtx.tx->vpout[i].get();

            if (!txout->IsType(OUTPUT_STANDARD))
                continue;

            isminetype mine = phdw->IsMine(txout);
            if (mine)
            {
                TransactionRecord sub(hash, nTime);
                CTxDestination address;
                sub.idx = i; // vout index
                sub.credit = txout->GetValue();
                sub.involvesWatchAddress = mine & ISMINE_WATCH_ONLY;
                if (ExtractDestination(*txout->GetPScriptPubKey(), address) && IsMine(*phdw, address))
                {
                    // Received by Bitcoin Address
                    if (wtx.IsCoinStake())
                        sub.type = TransactionRecord::Staked;
                    else
                        sub.type = TransactionRecord::RecvWithAddress;

                    sub.address = CBitcoinAddress(address).ToString();
                    //sub.address = EncodeDestination(address);
                } else
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
    } else
    {
        bool involvesWatchAddress = false;
        isminetype fAllFromMe = ISMINE_SPENDABLE;
        for (const CTxIn& txin : wtx.tx->vin)
        {
            if (txin.IsAnonInput())
                continue;
            isminetype mine = wallet->IsMine(txin);
            if(mine & ISMINE_WATCH_ONLY) involvesWatchAddress = true;
            if(fAllFromMe > mine) fAllFromMe = mine;
        }

        isminetype fAllToMe = ISMINE_SPENDABLE;
        for (const auto &txout : wtx.tx->vpout)
        {
            if (!txout->IsType(OUTPUT_STANDARD))
                continue;
            isminetype mine = phdw->IsMine(txout.get());
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

                if(phdw->IsMine(txout))
                {
                    // Ignore parts sent to self, as this is usually the change
                    // from a transaction sent back to our own address.
                    continue;
                }

                CTxDestination address;
                if (ExtractDestination(*txout->GetPScriptPubKey(), address))
                {
                    // Sent to Bitcoin Address
                    sub.type = TransactionRecord::SendToAddress;
                    sub.address = CBitcoinAddress(address).ToString();
                    //sub.address = EncodeDestination(address);
                } else
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

QList<TransactionRecord> TransactionRecord::decomposeTransaction(const CHDWallet *wallet, const uint256 &hash, const CTransactionRecord &rtx)
{
    QList<TransactionRecord> parts;
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
                    LogPrintf("%s: Warning, malformed vPath.", __func__);
                } else
                {
                    uint32_t sidx;
                    memcpy(&sidx, &r.vPath[1], 4);
                    CStealthAddress sx;
                    if (wallet->GetStealthByIndex(sidx, sx))
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
};

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
    }
    status.needsUpdate = false;
}

void TransactionRecord::updateStatus(CHDWallet *phdw, const CTransactionRecord &rtx)
{
    AssertLockHeld(cs_main);
    // Determine transaction status

    // Find the block the tx is in
    CBlockIndex* pindex = NULL;
    BlockMap::iterator mi = mapBlockIndex.find(rtx.blockHash);
    if (mi != mapBlockIndex.end())
        pindex = (*mi).second;

    // Sort order, unrecorded transactions sort to the top
    status.sortKey = strprintf("%010d-%01d-%010u-%03d",
        (pindex ? pindex->nHeight : std::numeric_limits<int>::max()),
        0,
        rtx.nTimeReceived,
        idx);
    //status.countsForBalance = wtx.IsTrusted() && !(wtx.GetBlocksToMaturity() > 0);
    status.countsForBalance = phdw->IsTrusted(hash, rtx.blockHash);
    status.depth = phdw->GetDepthInMainChain(rtx.blockHash, rtx.nIndex);
    status.cur_num_blocks = chainActive.Height();

    {
        if (status.depth < 0)
        {
            status.status = TransactionStatus::Conflicted;
        }
        else if (GetAdjustedTime() - rtx.nTimeReceived > 2 * 60 && phdw->GetRequestCount(hash, rtx) == 0)
        {
            status.status = TransactionStatus::Offline;
        }
        else if (status.depth == 0)
        {
            status.status = TransactionStatus::Unconfirmed;
            if (rtx.IsAbandoned())
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
};

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
