// Copyright (c) 2011-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/transactionrecord.h>

#include <chain.h>
#include <interfaces/wallet.h>
#include <key_io.h>
#include <wallet/ismine.h>
#include <wallet/wallet.h>

#include <stdint.h>

#include <QDateTime>

/* Return positive answer if transaction should be shown in list.
 */
bool TransactionRecord::showTransaction(const interfaces::WalletTx& wtx)
{
    // Ensures we show generated coins / mined transactions at depth 1
    if((wtx.is_coinbase || wtx.is_coinstake) && !wtx.is_in_main_chain)
    {
        return false;
    }
    return true;
}

/*
 * Decompose CWallet transaction to model transaction records.
 */
QList<TransactionRecord> TransactionRecord::decomposeTransaction(const CWallet* wallet, const interfaces::WalletTx& wtx)
{
    QList<TransactionRecord> parts;
    int64_t nTime = wtx.time;
    CAmount nCredit = wtx.credit;
    CAmount nDebit = wtx.debit;
    CAmount nNet = nCredit - nDebit;
    uint256 hash = wtx.tx->GetHash();
    std::map<std::string, std::string> mapValue = wtx.value_map;

    {
        LOCK(wallet->cs_wallet);
        // Decompose cold staking related transactions
        if (decomposeP2CS(wallet, wallet->GetWalletTx(hash), nCredit, nDebit, parts)) {
            return parts;
        }
    }

    if (nNet > 0 || wtx.is_coinbase || wtx.is_coinstake)
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
                if(wtx.is_coinstake) // Combine into single output for coinstake
                {
                    sub.idx = 1; // vout index
                    sub.credit = nNet;
                }
                else
                {
                    sub.idx = i; // vout index
                    sub.credit = txout.nValue;
                }
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
                if (wtx.is_coinbase || wtx.is_coinstake)
                {
                    // Generated
                    sub.type = TransactionRecord::Generated;
                }

                parts.append(sub);

                if(wtx.is_coinstake)
                    break; // Single output for coinstake
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
            std::string address;
            for (auto it = wtx.txout_address.begin(); it != wtx.txout_address.end(); ++it) {
                if (it != wtx.txout_address.begin()) address += ", ";
                address += EncodeDestination(*it);
            }

            CAmount nChange = wtx.change;
            parts.append(TransactionRecord(hash, nTime, TransactionRecord::SendToSelf, address, -(nDebit - nChange), nCredit - nChange));
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

bool TransactionRecord::decomposeP2CS(const CWallet* wallet, const CWalletTx* wtx,
                                           const CAmount& nCredit, const CAmount& nDebit,
                                           QList<TransactionRecord>& parts)
{
    if (wtx->HasP2CSOutputs()) {
        // Delegate tx.
        TransactionRecord sub(wtx->GetHash(), wtx->GetTxTime());
        sub.credit = nCredit;
        sub.debit = -nDebit;
        loadHotOrColdStakeOrContract(wallet, wtx, sub, !wtx->IsCoinStake());
        parts.append(sub);
        return true;
    } else if (wtx->HasP2CSInputs()) {
        // Delegation unlocked
        TransactionRecord sub(wtx->GetHash(), wtx->GetTxTime());
        loadUnlockColdStake(wallet, wtx, sub);
        parts.append(sub);
        return true;
    }
    return false;
}

void TransactionRecord::loadUnlockColdStake(const CWallet* wallet, const CWalletTx* wtx, TransactionRecord& record)
{
    record.involvesWatchAddress = false;

    // Get the p2cs
    const CScript* p2csScript = nullptr;
    bool isSpendable = false;

    for (const auto &input : wtx->tx->vin) {
        const CWalletTx* wallettx = wallet->GetWalletTx(input.prevout.hash);
        if (wallettx && wallettx->tx->vout[input.prevout.n].scriptPubKey.IsPayToColdStaking()) {
            p2csScript = &wallettx->tx->vout[input.prevout.n].scriptPubKey;
            isSpendable = wallet->IsMine(input) & ISMINE_SPENDABLE_ALL;
            break;
        }
    }

    if (isSpendable) {
        // owner unlocked the cold stake
        record.type = TransactionRecord::P2CSUnlockOwner;
        record.debit = -(wtx->GetStakeDelegationDebit());
        record.credit = wtx->GetCredit(ISMINE_ALL);
    } else {
        // hot node watching the unlock
        record.type = TransactionRecord::P2CSUnlockStaker;
        record.debit = -(wtx->GetColdStakingDebit());
        record.credit = -(wtx->GetColdStakingCredit());
    }

    // Extract and set the owner address
    if (p2csScript) {
        ExtractAddress(*p2csScript, false, record.address);
    }
}

void TransactionRecord::loadHotOrColdStakeOrContract(
        const CWallet* wallet,
        const CWalletTx* wtx,
        TransactionRecord& record,
        bool isContract)
{
    record.involvesWatchAddress = false;

    // Get the p2cs
    CTxOut p2csUtxo;
    unsigned int p2csUtxoIndex = 0;
    for (const auto & txout : wtx->tx->vout) {
        if (txout.scriptPubKey.IsPayToColdStaking()) {
            p2csUtxo = txout;
            break;
        }
        p2csUtxoIndex++;
    }

    bool isSpendable = wallet->IsMine(p2csUtxo) & ISMINE_SPENDABLE_DELEGATED;
    bool isFromMe = wtx->IsFromMe(ISMINE_ALL);
    bool isSpent = wallet->IsSpent(wtx->GetHash(), p2csUtxoIndex);

    if (isContract) {
        if (isSpendable && isFromMe) {
            // Wallet delegating balance
            record.type = isSpent ? TransactionRecord::P2CSSpentDelegationSentOwner : TransactionRecord::P2CSDelegationSentOwner;
            record.delegated = wtx->GetCredit(ISMINE_SPENDABLE_DELEGATED);
        } else if (isFromMe){
            // Wallet delegating balance and transfering ownership
            record.type = isSpent ? TransactionRecord::P2CSSpentDelegationSent : TransactionRecord::P2CSDelegationSent;
            record.delegated = wtx->GetCredit(ISMINE_SPENDABLE_DELEGATED);
        } else {
            // Wallet receiving a delegation
            record.type = TransactionRecord::P2CSDelegation;
        }
    } else {
        // Stake
        if (isSpendable) {
            // Offline wallet receiving an stake due a delegation
            record.type = isSpent ? TransactionRecord::SpentStakeDelegated : TransactionRecord::StakeDelegated;
            record.credit = wtx->GetCredit(ISMINE_SPENDABLE_DELEGATED);
            record.debit = -(wtx->GetDebit(ISMINE_SPENDABLE_DELEGATED));
            record.delegated = record.credit;
        } else {
            // Online wallet receiving an stake due to a received utxo delegation that won a block.
            record.type = TransactionRecord::StakeHot;
            record.credit = wtx->GetCredit(ISMINE_SPENDABLE);
            record.debit = -(wtx->GetDebit(ISMINE_SPENDABLE));
        }
    }

    // Extract and set the owner address
    ExtractAddress(p2csUtxo.scriptPubKey, false, record.address);
}

bool TransactionRecord::ExtractAddress(const CScript& scriptPubKey, bool fColdStake, std::string& addressStr) {
    CTxDestination address;
    if (!ExtractDestination(scriptPubKey, address, NULL, fColdStake)) {
        // this shouldn't happen..
        addressStr = "No available address";
        return false;
    } else {
        addressStr = EncodeDestination(address);
        return true;
    }
}

void TransactionRecord::updateStatus(const interfaces::WalletTxStatus& wtx, const uint256& block_hash, int numBlocks, int64_t block_time)
{
    // Determine transaction status

    // Sort order, unrecorded transactions sort to the top
    status.sortKey = strprintf("%010d-%01d-%010u-%03d",
        wtx.block_height,
        (wtx.is_coinbase || wtx.is_coinstake) ? 1 : 0,
        wtx.time_received,
        idx);
    status.countsForBalance = wtx.is_trusted && !(wtx.blocks_to_maturity > 0);
    status.depth = wtx.depth_in_main_chain;
    status.m_cur_block_hash = block_hash;

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
    else if(type == TransactionRecord::Generated ||
            type == TransactionRecord::StakeDelegated ||
            type == TransactionRecord::SpentStakeDelegated ||
            type == TransactionRecord::StakeHot)
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
    status.needsUpdate = false;
}

bool TransactionRecord::statusUpdateNeeded(const uint256& block_hash) const
{
    assert(!block_hash.IsNull());
    return status.m_cur_block_hash != block_hash || status.needsUpdate;
}

QString TransactionRecord::getTxHash() const
{
    return QString::fromStdString(hash.ToString());
}

int TransactionRecord::getOutputIndex() const
{
    return idx;
}
