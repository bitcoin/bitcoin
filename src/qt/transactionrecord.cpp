// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "transactionrecord.h"

#include "base58.h"
#include "wallet.h"

#include <stdint.h>

/* Return positive answer if transaction should be shown in list.
 */
bool Credits_TransactionRecord::showTransaction(const Bitcredit_CWalletTx &wtx)
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

void SetupNoneFromMeDepositSub(const Bitcredit_CWalletTx &wtx, const unsigned int& nOut, Credits_TransactionRecord &sub, const Bitcredit_CWallet *keyholder_wallet) {
    if(wtx.IsDeposit()) {
    	if(nOut == 0) {
    		sub.type = Credits_TransactionRecord::Deposit;
    	} else {
    		sub.type = Credits_TransactionRecord::DepositChange;
    	}

    	const int64_t nValue = wtx.vout[nOut].nValue;

		if(keyholder_wallet->IsMine(wtx.vout[nOut]))
		{
	    	sub.debit = 0;
	    	sub.credit = nValue;
		} else {
	    	sub.debit = 0;
	    	sub.credit = 0;
		}
    }
}

void SetupAllFromMeDepositSub(const Bitcredit_CWalletTx &wtx, const unsigned int& nOut, Credits_TransactionRecord &sub, const Bitcredit_CWallet *keyholder_wallet) {
    if(wtx.IsDeposit()) {
    	if(nOut == 0) {
    		sub.type = Credits_TransactionRecord::Deposit;
    	} else {
    		sub.type = Credits_TransactionRecord::DepositChange;
    	}

    	const int64_t nValue = wtx.vout[nOut].nValue;

		if(keyholder_wallet->IsMine(wtx.vout[nOut]))
		{
	    	sub.debit = -nValue;
	    	sub.credit = nValue;
		} else {
	    	sub.debit = -nValue;
	    	sub.credit = 0;
		}
    }
}

void SetupAllFromMeAllToMeDepositSub(const Bitcredit_CWalletTx &wtx, const unsigned int& nOut, Credits_TransactionRecord &sub) {
    if(wtx.IsDeposit()) {
    	if(nOut == 0) {
    		sub.type = Credits_TransactionRecord::Deposit;
    	} else {
    		sub.type = Credits_TransactionRecord::DepositChange;
    	}

    	const int64_t nValue = wtx.vout[nOut].nValue;
    	sub.debit = -nValue;
    	sub.credit = nValue;

        CTxDestination address;
        if (ExtractDestination(wtx.vout[nOut].scriptPubKey, address))
        {
            // Sent to Credits Address
            sub.address = CBitcoinAddress(address).ToString();
        }
        else
        {
            std::map<std::string, std::string> mapValue = wtx.mapValue;
            // Sent to IP, or other non-address transaction like OP_EVAL
            sub.address = mapValue["to"];
        }
    }
}
/*
 * Decompose CWallet transaction to model transaction records.
 */
QList<Credits_TransactionRecord> Credits_TransactionRecord::decomposeTransaction(const Bitcredit_CWallet *keyholder_wallet, const Bitcredit_CWalletTx &wtx)
{
    //The decomposed transactions should not be affected by prepared deposits. Passing in empty list.
    map<uint256, set<int> > mapPreparedDepositTxInPoints;

    QList<Credits_TransactionRecord> parts;
    int64_t nTime = wtx.GetTxTime();
    int64_t nCredit = wtx.GetCredit(mapPreparedDepositTxInPoints, keyholder_wallet);
    int64_t nDebit = wtx.GetDebit(keyholder_wallet);
    int64_t nNet = nCredit - nDebit;
    uint256 hash = wtx.GetHash();
    std::map<std::string, std::string> mapValue = wtx.mapValue;

    if (nNet > 0 || wtx.IsCoinBase())
    {
        //
        // Credit
        //
		for (unsigned int nOut = 0; nOut < wtx.vout.size(); nOut++)
		{
			const CTxOut& txout = wtx.vout[nOut];

            if(keyholder_wallet->IsMine(txout))
            {
                Credits_TransactionRecord sub(hash, nTime);
                CTxDestination address;
                sub.idx = parts.size(); // sequence number
                sub.credit = txout.nValue;
                if (ExtractDestination(txout.scriptPubKey, address) && IsMine(*keyholder_wallet, address))
                {
                    // Received by Credits Address
                    sub.type = Credits_TransactionRecord::RecvWithAddress;
                    sub.address = CBitcoinAddress(address).ToString();
                }
                else
                {
                    // Received by IP connection (deprecated features), or a multisignature or other non-simple transaction
                    sub.type = Credits_TransactionRecord::RecvFromOther;
                    sub.address = mapValue["from"];
                }
                if (wtx.IsCoinBase())
                {
                    // Generated
                    sub.type = Credits_TransactionRecord::Generated;
                }
                else if (wtx.IsDeposit())
                {
                	if(nOut == 0) {
                		sub.type = Credits_TransactionRecord::Deposit;
                	} else {
                		sub.type = Credits_TransactionRecord::DepositChange;
                	}
                }
                parts.append(sub);
            }
        }
    }
    else
    {
        bool fAllFromMe = true;
        BOOST_FOREACH(const Bitcredit_CTxIn& txin, wtx.vin)
            fAllFromMe = fAllFromMe && keyholder_wallet->IsMine(txin);

        bool fAllToMe = true;
        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
            fAllToMe = fAllToMe && keyholder_wallet->IsMine(txout);

        if (fAllFromMe && fAllToMe)
        {
            // Payment to self
            int64_t nChange = wtx.GetChange(keyholder_wallet);
            Credits_TransactionRecord sub(hash, nTime, Credits_TransactionRecord::SendToSelf, "", -(nDebit - nChange), nCredit - nChange);
            SetupAllFromMeAllToMeDepositSub(wtx, 0, sub);
            parts.append(sub);

            if(wtx.IsDeposit() && wtx.vout.size() == 2) {
                Credits_TransactionRecord sub(hash, nTime, Credits_TransactionRecord::SendToSelf, "", -(nDebit - nChange), nCredit - nChange);
                SetupAllFromMeAllToMeDepositSub(wtx, 1, sub);
                parts.append(sub);
            }

        }
        else if (fAllFromMe)
        {
            //
            // Debit
            //
            int64_t nTxFee = nDebit - wtx.GetValueOut();

            for (unsigned int nOut = 0; nOut < wtx.vout.size(); nOut++)
            {
                const CTxOut& txout = wtx.vout[nOut];
                Credits_TransactionRecord sub(hash, nTime);
                sub.idx = parts.size();

                if(!wtx.IsDeposit()) {
					if(keyholder_wallet->IsMine(txout))
					{
						// Ignore parts sent to self, as this is usually the change
						// from a transaction sent back to our own address.
						continue;
					}
                }

                CTxDestination address;
                if (ExtractDestination(txout.scriptPubKey, address))
                {
                    // Sent to Credits Address
                    sub.type = Credits_TransactionRecord::SendToAddress;
                    sub.address = CBitcoinAddress(address).ToString();
                }
                else
                {
                    // Sent to IP, or other non-address transaction like OP_EVAL
                    sub.type = Credits_TransactionRecord::SendToOther;
                    sub.address = mapValue["to"];
                }

                int64_t nValue = txout.nValue;
                /* Add fee to first output */
                if (nTxFee > 0)
                {
                    nValue += nTxFee;
                    nTxFee = 0;
                }
                sub.debit = -nValue;

                SetupAllFromMeDepositSub(wtx, nOut, sub, keyholder_wallet);
                parts.append(sub);

            }
        }
        else
        {
            //
            // Mixed debit transaction, can't break down payees
            //
        	if(wtx.IsDeposit()) {
				if(keyholder_wallet->IsMine(wtx.vout[0]))
				{
					Credits_TransactionRecord sub(hash, nTime, Credits_TransactionRecord::Other, "", nNet, 0);
					SetupNoneFromMeDepositSub(wtx, 0, sub, keyholder_wallet);
					parts.append(sub);
				}
				if(wtx.vout.size() == 2) {
					if(keyholder_wallet->IsMine(wtx.vout[1]))
					{
						Credits_TransactionRecord sub(hash, nTime, Credits_TransactionRecord::Other, "", nNet, 0);
						SetupNoneFromMeDepositSub(wtx, 1, sub, keyholder_wallet);
						parts.append(sub);
					}
				}
        	} else {
				Credits_TransactionRecord sub(hash, nTime, Credits_TransactionRecord::Other, "", nNet, 0);
				parts.append(sub);
        	}

        }
    }

    return parts;
}

void Credits_TransactionRecord::updateStatus(const Bitcredit_CWalletTx &wtx)
{
    AssertLockHeld(bitcredit_mainState.cs_main);
    // Determine transaction status

    // Find the block the tx is in
    Bitcredit_CBlockIndex* pindex = NULL;
    std::map<uint256, Bitcredit_CBlockIndex*>::iterator mi = bitcredit_mapBlockIndex.find(wtx.hashBlock);
    if (mi != bitcredit_mapBlockIndex.end())
        pindex = (*mi).second;

    // Sort order, unrecorded transactions sort to the top
    status.sortKey = strprintf("%010d-%01d-%010u-%03d",
        (pindex ? pindex->nHeight : std::numeric_limits<int>::max()),
        (wtx.IsCoinBase() ? 1 : 0),
        wtx.nTimeReceived,
        idx);
    status.countsForBalance = wtx.IsTrusted() && !(wtx.GetBlocksToMaturity() > 0);
    status.depth = wtx.GetDepthInMainChain();
    status.cur_num_blocks = bitcredit_chainActive.Height();

    if (!Bitcredit_IsFinalTx(wtx, bitcredit_chainActive.Height() + 1))
    {
        if (wtx.nLockTime < BITCREDIT_LOCKTIME_THRESHOLD)
        {
            status.status = Bitcredit_TransactionStatus::OpenUntilBlock;
            status.open_for = wtx.nLockTime - bitcredit_chainActive.Height();
        }
        else
        {
            status.status = Bitcredit_TransactionStatus::OpenUntilDate;
            status.open_for = wtx.nLockTime;
        }
    }
    // For generated transactions, determine maturity
    else if(type == Credits_TransactionRecord::Generated)
    {
        if (wtx.GetBlocksToMaturity() > 0)
        {
            status.status = Bitcredit_TransactionStatus::Immature;

            if (wtx.IsInMainChain())
            {
                status.matures_in = wtx.GetBlocksToMaturity();

                // Check if the block was requested by anyone
                if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
                    status.status = Bitcredit_TransactionStatus::MaturesWarning;
            }
            else
            {
                status.status = Bitcredit_TransactionStatus::NotAccepted;
            }
        }
        else
        {
            status.status = Bitcredit_TransactionStatus::Confirmed;
        }
    }
    else if(type == Credits_TransactionRecord::Deposit)
    {

        if (wtx.GetFirstDepositOutBlocksToMaturity() > 0)
        {
            status.status = Bitcredit_TransactionStatus::ImmatureDeposit;

            if (wtx.IsInMainChain())
            {
                status.matures_in = wtx.GetFirstDepositOutBlocksToMaturity();

                // Check if the block was requested by anyone
                if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
                    status.status = Bitcredit_TransactionStatus::MaturesWarning;
            }
            else
            {
                status.status = Bitcredit_TransactionStatus::NotAccepted;
            }
        }
        else
        {
            status.status = Bitcredit_TransactionStatus::Confirmed;
        }
    }
    else if(type == Credits_TransactionRecord::DepositChange)
    {

        if (wtx.GetSecondDepositOutBlocksToMaturity() > 0)
        {
            status.status = Bitcredit_TransactionStatus::ImmatureDepositChange;

            if (wtx.IsInMainChain())
            {
                status.matures_in = wtx.GetSecondDepositOutBlocksToMaturity();

                // Check if the block was requested by anyone
                if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
                    status.status = Bitcredit_TransactionStatus::MaturesWarning;
            }
            else
            {
                status.status = Bitcredit_TransactionStatus::NotAccepted;
            }
        }
        else
        {
            status.status = Bitcredit_TransactionStatus::Confirmed;
        }
    }
    else
    {
        if (status.depth < 0)
        {
            status.status = Bitcredit_TransactionStatus::Conflicted;
        }
        else if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
        {
            status.status = Bitcredit_TransactionStatus::Offline;
        }
        else if (status.depth == 0)
        {
            status.status = Bitcredit_TransactionStatus::Unconfirmed;
        }
        else if (status.depth < RecommendedNumConfirmations)
        {
            status.status = Bitcredit_TransactionStatus::Confirming;
        }
        else
        {
            status.status = Bitcredit_TransactionStatus::Confirmed;
        }
    }

}

bool Credits_TransactionRecord::statusUpdateNeeded()
{
    AssertLockHeld(bitcredit_mainState.cs_main);
    return status.cur_num_blocks != bitcredit_chainActive.Height();
}

QString Credits_TransactionRecord::getTxID() const
{
    return formatSubTxId(hash, idx);
}

QString Credits_TransactionRecord::formatSubTxId(const uint256 &hash, int vout)
{
    return QString::fromStdString(hash.ToString() + strprintf("-%03d", vout));
}

