// Copyright (c) 2011-2015 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "transactionrecord.h"

#include "base58.h"
#include "consensus/consensus.h"
#include "main.h"
#include "timedata.h"
#include "wallet/wallet.h"
#include <stdint.h>

#include <boost/foreach.hpp>
using namespace std;
// SYSCOIN
#include "offer.h"
extern int GetSyscoinTxVersion();
extern bool IsSyscoinDataOutput(const CTxOut& out);
extern bool IsSyscoinTxMine(const CTransaction& tx, const string &type);
extern std::string stringFromVch(const std::vector<unsigned char> &vch);
extern bool DecodeAndParseSyscoinTx(const CTransaction& tx, int& op, int& nOut, vector<vector<unsigned char> >& vvch);
enum {RECV=0, SEND=1};
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
static bool CreateSyscoinTransactionRecord(TransactionRecord& sub, int op, const vector<vector<unsigned char> > &vvchArgs, const CWalletTx &wtx, int type)
{
	switch(op)
	{
	case OP_ALIAS_ACTIVATE:
		if(type == SEND)
			sub.type = TransactionRecord::AliasActivate;
		break;
	case OP_ALIAS_UPDATE:
		if(type == SEND)
			sub.type = (IsSyscoinTxMine(wtx, "alias")) ? TransactionRecord::AliasUpdate : TransactionRecord::AliasTransfer;	
		else if(type == RECV)
			sub.type = TransactionRecord::AliasRecv;
		break;
	case OP_OFFER_ACTIVATE:
		if(type == SEND)
			sub.type = TransactionRecord::OfferActivate;
		break;
	case OP_OFFER_UPDATE:
		if(type == SEND)
			sub.type = TransactionRecord::OfferUpdate;
		break;
	case OP_OFFER_ACCEPT:
		if(type == SEND)
			sub.type = TransactionRecord::OfferAccept;
		else if(type == RECV)
			sub.type = TransactionRecord::OfferAcceptRecv;
		break;
	case OP_CERT_ACTIVATE:
		if(type == SEND)
			sub.type = TransactionRecord::CertActivate;
		break;
	case OP_CERT_UPDATE:
		if(type == SEND)
			sub.type = TransactionRecord::CertUpdate;
		break;
	case OP_CERT_TRANSFER:
		if(type == SEND)
			sub.type = TransactionRecord::CertTransfer;
		else if(type == RECV)
			sub.type = TransactionRecord::CertRecv;
		break;
	case OP_ESCROW_ACTIVATE:
		if(type == SEND || type == RECV)
			sub.type = TransactionRecord::EscrowActivate;
		break;
	case OP_ESCROW_RELEASE:
		if(type == SEND  || type == RECV)
			sub.type = TransactionRecord::EscrowRelease;
		break;
	case OP_ESCROW_COMPLETE:
		if(type == SEND || type == RECV)
			sub.type = TransactionRecord::EscrowComplete;
		break;
	case OP_ESCROW_REFUND:
		if(type == SEND)
			sub.type = TransactionRecord::EscrowRefund;
		else if(type == RECV)
			sub.type = TransactionRecord::EscrowRefundRecv;
		break;
	case OP_MESSAGE_ACTIVATE:
		if(type == SEND)
			sub.type = TransactionRecord::MessageActivate;
		else if(type == RECV)
			sub.type = TransactionRecord::MessageRecv;
		break;
	default:
		return false;
	}
	sub.address = stringFromVch(vvchArgs[0]);
	return true;
}
static bool CreateSyscoinTransactions(const CWallet *wallet, const CWalletTx& wtx, QList<TransactionRecord>& parts, const int64_t &nTime, const CAmount &nNet, const int type)
{
	if(wtx.nVersion != GetSyscoinTxVersion())
		return false;	
	uint256 hash = wtx.GetHash();
	BOOST_FOREACH(const CTxOut& txout, wtx.vout)
	{
        vector<vector<unsigned char> > vvchArgs;
        int op, nOut;
		isminetype mine = wallet->IsMine(txout);
		if(mine)
		{
			// there should only be one data carrying syscoin output per transaction, but there may be more than 1 syscoin utxo in a transaction
			// we want to display the data carrying one and not the empty utxo		
			if(DecodeAndParseSyscoinTx(wtx, op, nOut, vvchArgs))
			{
				TransactionRecord sub(hash, nTime);
				if(!CreateSyscoinTransactionRecord(sub, op, vvchArgs, wtx, type))
					return false;
				sub.idx = parts.size(); // sequence number
				if(type == RECV)
					sub.credit = nNet;
				else if(type == SEND)
					sub.debit = nNet;
				parts.append(sub);
				return true;
			}			
		}
	}
	return false;
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
	// SYSCOIN Check if tx is a syscoin service
    vector<vector<unsigned char> > vvchArgs;
    int op, nOut;
	op = 0;

    if (nNet > 0 || wtx.IsCoinBase())
    {
        //
        // Credit
        //
		// SYSCOIN - this should be a received service
		if(!CreateSyscoinTransactions(wallet, wtx, parts, nTime, nNet, RECV))
		{
			BOOST_FOREACH(const CTxOut& txout, wtx.vout)
			{
				isminetype mine = wallet->IsMine(txout);
				if(mine)
				{
					TransactionRecord sub(hash, nTime);
					CTxDestination address;
					sub.idx = parts.size(); // sequence number
					sub.credit = txout.nValue;
					sub.involvesWatchAddress = mine & ISMINE_WATCH_ONLY;
					if (ExtractDestination(txout.scriptPubKey, address) && IsMine(*wallet, address))
					{
						// Received by Syscoin Address
						sub.type = TransactionRecord::RecvWithAddress;
						// SYSCOIN show alias in record
						CSyscoinAddress sysAddress = CSyscoinAddress(address);
						sysAddress = CSyscoinAddress(sysAddress.ToString());
						if(sysAddress.isAlias)
							sub.address = sysAddress.aliasName;
						else
							sub.address = sysAddress.ToString();
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
		
    }
    else
    {
        bool involvesWatchAddress = false;
        isminetype fAllFromMe = ISMINE_SPENDABLE;
        BOOST_FOREACH(const CTxIn& txin, wtx.vin)
        {
            isminetype mine = wallet->IsMine(txin);
            if(mine & ISMINE_WATCH_ONLY) involvesWatchAddress = true;
            if(fAllFromMe > mine) fAllFromMe = mine;
        }

        isminetype fAllToMe = ISMINE_SPENDABLE;
        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
        {
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
			// SYSCOIN - this should be a new service you've created
			if(!CreateSyscoinTransactions(wallet, wtx, parts, nTime, nNet, SEND))
			{
				CAmount nTxFee = nDebit - wtx.GetValueOut();

				for (unsigned int nOut = 0; nOut < wtx.vout.size(); nOut++)
				{
					const CTxOut& txout = wtx.vout[nOut];
					TransactionRecord sub(hash, nTime);
					sub.idx = parts.size();
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
						// Sent to Syscoin Address
						sub.type = TransactionRecord::SendToAddress;
						// SYSCOIN show alias in record
						CSyscoinAddress sysAddress = CSyscoinAddress(address);
						sysAddress = CSyscoinAddress(sysAddress.ToString());
						if(sysAddress.isAlias)
							sub.address = sysAddress.aliasName;
						else
							sub.address = sysAddress.ToString();
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
        }
        else
        {
            //
            // Mixed debit transaction, can't break down payees
            //
			// SYSCOIN - this should be a new service you've created
			if(!CreateSyscoinTransactions(wallet, wtx, parts, nTime, nNet, SEND))
			{
				parts.append(TransactionRecord(hash, nTime, TransactionRecord::Other, "", nNet, 0));
				parts.last().involvesWatchAddress = involvesWatchAddress;
			}
        }
    }

    return parts;
}

void TransactionRecord::updateStatus(const CWalletTx &wtx)
{
    AssertLockHeld(cs_main);
    // Determine transaction status

    // Find the block the tx is in
    CBlockIndex* pindex = NULL;
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

QString TransactionRecord::getTxID() const
{
    return formatSubTxId(hash, idx);
}

QString TransactionRecord::formatSubTxId(const uint256 &hash, int vout)
{
    return QString::fromStdString(hash.ToString() + strprintf("-%03d", vout));
}

