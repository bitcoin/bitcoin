#include "transactionrecord.h"

#include "wallet.h"
#include "base58.h"

#include "gameunitsgui.h"

/* Return positive answer if transaction should be shown in list.
 */
bool TransactionRecord::showTransaction(const CWalletTx &wtx)
{
    if (wtx.IsCoinBase() && !wtx.IsInMainChain()) // Ensures we show generated coins / mined transactions at depth 1
        return false;

    return true;
}

QString TransactionRecord::getTypeLabel(const int &type)
{
    switch(type)
    {
    case RecvWithAddress:
        return GameunitsGUI::tr("Received with");
    case RecvFromOther:
        return GameunitsGUI::tr("Received from");
    case SendToAddress:
    case SendToOther:
        return GameunitsGUI::tr("Sent to");
    case SendToSelf:
        return GameunitsGUI::tr("Payment to yourself");
    case Generated:
        return GameunitsGUI::tr("Mined");
    case RecvGameunitsX:
        return GameunitsGUI::tr("Received GAMEUNITSX");
    case SendGameunitsX:
        return GameunitsGUI::tr("Sent GAMEUNITSX");
    case Other:
        return GameunitsGUI::tr("Other");
    default:
        return "";
    }
}

QString TransactionRecord::getTypeShort(const int &type)
{
    switch(type)
    {
    case TransactionRecord::Generated:
        return "mined";
    case TransactionRecord::RecvWithAddress:
    case TransactionRecord::RecvFromOther:
    case TransactionRecord::RecvGameunitsX:
        return "input";
    case TransactionRecord::SendToAddress:
    case TransactionRecord::SendToOther:
    case TransactionRecord::SendGameunitsX:
        return "output";
    default:
        return "inout";
    }
}

/*
 * Decompose CWallet transaction to model transaction records.
 */
QList<TransactionRecord> TransactionRecord::decomposeTransaction(const CWallet *wallet, const CWalletTx &wtx)
{
    QList<TransactionRecord> parts;
    int64_t nTime = wtx.GetTxTime();

    int64_t nCredGAMEUNITS, nCredGameunitsX;
    wtx.GetCredit(nCredGAMEUNITS, nCredGameunitsX, true);
    int64_t nCredit = nCredGAMEUNITS + nCredGameunitsX;
    int64_t nDebit = wtx.GetDebit();
    int64_t nNet = nCredit - nDebit;
    uint256 hash = wtx.GetHash(), hashPrev = 0;
    std::map<std::string, std::string> mapValue = wtx.mapValue;

    char cbuf[256];

    if (wtx.nVersion == ANON_TXN_VERSION)
    {
        if (nNet > 0 && nCredGameunitsX > 0)
        {
            // -- credit
            TransactionRecord sub(hash, nTime, TransactionRecord::RecvGameunitsX, "", "", nNet, 0);

            for (unsigned int nOut = 0; nOut < wtx.vout.size(); nOut++)
            {
                // display 1st transaction
                snprintf(cbuf, sizeof(cbuf), "n_%u", nOut);
                mapValue_t::const_iterator mi = wtx.mapValue.find(cbuf);
                if (mi != wtx.mapValue.end() && !mi->second.empty())
                {
                    sub.narration = mi->second;
                    break;
                };
            };

            parts.append(sub);
            return parts;
        } else
        if (nNet <= 0)
        {
            // -- debit
            TransactionRecord sub(hash, nTime, TransactionRecord::SendGameunitsX, "", "", nNet, 0);

            for (unsigned int nOut = 0; nOut < wtx.vout.size(); nOut++)
            {
                // display 1st transaction
                snprintf(cbuf, sizeof(cbuf), "n_%u", nOut);
                mapValue_t::const_iterator mi = wtx.mapValue.find(cbuf);
                if (mi != wtx.mapValue.end() && !mi->second.empty())
                {
                    sub.narration = mi->second;
                    break;
                }
            };

            parts.append(sub);
            return parts;
        };

        // continue on
    };


    if (nNet > 0 || wtx.IsCoinBase() || wtx.IsCoinStake())
    {
        //
        // Credit
        //

        for (unsigned int nOut = 0; nOut < wtx.vout.size(); nOut++)
        {
            const CTxOut& txout = wtx.vout[nOut];

            if (wtx.nVersion == ANON_TXN_VERSION
                && txout.IsAnonOutput())
            {
                const CScript &s = txout.scriptPubKey;
                CKeyID ckidD = CPubKey(&s[2+1], 33).GetID();

                if (wallet->HaveKey(ckidD))
                {
                    TransactionRecord sub(hash, nTime);
                    sub.idx = parts.size(); // sequence number

                    sub.credit = txout.nValue;

                    sub.type = TransactionRecord::RecvGameunitsX;
                    sub.address = CBitcoinAddress(ckidD).ToString();
                    //sub.address = wallet->mapAddressBook[ckidD]

                    snprintf(cbuf, sizeof(cbuf), "n_%u", nOut);
                    mapValue_t::const_iterator mi = wtx.mapValue.find(cbuf);
                    if (mi != wtx.mapValue.end() && !mi->second.empty())
                        sub.narration = mi->second;

                    parts.append(sub);
                };
            };

            if (wallet->IsMine(txout))
            {
                TransactionRecord sub(hash, nTime);
                sub.idx = parts.size(); // sequence number

                CTxDestination address;

                sub.credit = txout.nValue;
                if (ExtractDestination(txout.scriptPubKey, address) && IsMine(*wallet, address))
                {
                    // Received by Bitcoin Address
                    sub.type = TransactionRecord::RecvWithAddress;
                    sub.address = CBitcoinAddress(address).ToString();
                } else
                {
                    // Received by IP connection (deprecated features), or a multisignature or other non-simple transaction
                    sub.type = TransactionRecord::RecvFromOther;
                    sub.address = mapValue["from"];
                }

                snprintf(cbuf, sizeof(cbuf), "n_%u", nOut);
                mapValue_t::const_iterator mi = wtx.mapValue.find(cbuf);
                if (mi != wtx.mapValue.end() && !mi->second.empty())
                    sub.narration = mi->second;

                if (wtx.IsCoinBase())
                {
                    // Generated (proof-of-work)
                    sub.type = TransactionRecord::Generated;
                };

                if (wtx.IsCoinStake())
                {
                    // Generated (proof-of-stake)

                    if (hashPrev == hash)
                        continue; // last coinstake output

                    sub.type = TransactionRecord::Generated;
                    sub.credit = nNet > 0 ? nNet : wtx.GetValueOut() - nDebit;
                    hashPrev = hash;
                };

                parts.append(sub);
            }
        }
    } else
    {
        bool fAllFromMe = true;
        BOOST_FOREACH(const CTxIn& txin, wtx.vin)
        {
            if (wtx.nVersion == ANON_TXN_VERSION
                && txin.IsAnonInput())
            {
                std::vector<uint8_t> vchImage;
                txin.ExtractKeyImage(vchImage);

                CWalletDB walletdb(wallet->strWalletFile, "r");
                COwnedAnonOutput oao;
                if (!walletdb.ReadOwnedAnonOutput(vchImage, oao))
                {
                    fAllFromMe = false;
                    break; // display as send/recv gameunits 
                };
                continue;
            };

            if (wallet->IsMine(txin))
                continue;
            fAllFromMe = false;
            break;
        };

        bool fAllToMe = true;
        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
        {
            if (wtx.nVersion == ANON_TXN_VERSION
                && txout.IsAnonOutput())
            {
                fAllToMe = false;
                break; // display as send/recv gameunitsx
            }
            opcodetype firstOpCode;
            CScript::const_iterator pc = txout.scriptPubKey.begin();
            if (txout.scriptPubKey.GetOp(pc, firstOpCode)
                && firstOpCode == OP_RETURN)
                continue;
            if (wallet->IsMine(txout))
                continue;

            fAllToMe = false;
            break;
        };

        if (fAllFromMe && fAllToMe)
        {
            // Payment to self
            int64_t nChange = wtx.GetChange();

            std::string narration("");
            mapValue_t::const_iterator mi;
            for (mi = wtx.mapValue.begin(); mi != wtx.mapValue.end(); ++mi)
            {
                if (mi->first.compare(0, 2, "n_") != 0)
                    continue;
                narration = mi->second;
                break;
            };

            parts.append(TransactionRecord(hash, nTime, TransactionRecord::SendToSelf, "", narration,
                            -(nDebit - nChange), nCredit - nChange));
        } else
        if (fAllFromMe)
        {
            //
            // Debit
            //
            int64_t nTxFee = nDebit - wtx.GetValueOut();


            for (unsigned int nOut = 0; nOut < wtx.vout.size(); nOut++)
            {
                const CTxOut& txout = wtx.vout[nOut];

                TransactionRecord sub(hash, nTime);
                sub.idx = parts.size();

                if (wtx.nVersion == ANON_TXN_VERSION
                    && txout.IsAnonOutput())
                {
                    const CScript &s = txout.scriptPubKey;
                    CKeyID ckidD = CPubKey(&s[2+1], 33).GetID();

                    CTxDestination address;
                    sub.idx = parts.size(); // sequence number
                    sub.credit = txout.nValue;

                    sub.type = TransactionRecord::SendGameunitsX;
                    sub.address = CBitcoinAddress(ckidD).ToString();

                } else
                {
                    opcodetype firstOpCode;
                    CScript::const_iterator pc = txout.scriptPubKey.begin();
                    if (txout.scriptPubKey.GetOp(pc, firstOpCode)
                        && firstOpCode == OP_RETURN)
                        continue;

                    if (wallet->IsMine(txout))
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
                        sub.address = CBitcoinAddress(address).ToString();
                    } else
                    {
                        // Sent to IP, or other non-address transaction like OP_EVAL
                        sub.type = TransactionRecord::SendToOther;
                        sub.address = mapValue["to"];
                    }
                };

                snprintf(cbuf, sizeof(cbuf), "n_%u", nOut);
                mapValue_t::const_iterator mi = wtx.mapValue.find(cbuf);
                if (mi != wtx.mapValue.end() && !mi->second.empty())
                    sub.narration = mi->second;

                int64_t nValue = txout.nValue;
                /* Add fee to first output */
                if (nTxFee > 0)
                {
                    nValue += nTxFee;
                    nTxFee = 0;
                }
                sub.debit = -nValue;

                parts.append(sub);
            }
        } else
        {
            //
            // Mixed debit transaction, can't break down payees
            //
            TransactionRecord sub(hash, nTime, TransactionRecord::Other, "", "", nNet, 0);
            /*
            for (unsigned int nOut = 0; nOut < wtx.vout.size(); nOut++)
            {
                // display 1st transaction
                snprintf(cbuf, sizeof(cbuf), "n_%u", nOut);
                mapValue_t::const_iterator mi = wtx.mapValue.find(cbuf);
                if (mi != wtx.mapValue.end() && !mi->second.empty())
                    sub.narration = mi->second;
                break;
            };
            */
            parts.append(sub);
        }
    }

    return parts;
}

void TransactionRecord::updateStatus(const CWalletTx &wtx)
{
    AssertLockHeld(cs_main);
    // Determine transaction status

    int nHeight = std::numeric_limits<int>::max();

    // Find the block the tx is in
    if (nNodeMode == NT_FULL)
    {
        CBlockIndex* pindex = NULL;
        std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(wtx.hashBlock);
        if (mi != mapBlockIndex.end())
        {
            pindex = (*mi).second;
            nHeight = pindex->nHeight;
        };
    } else
    {
        CBlockThinIndex* pindex = NULL;
        std::map<uint256, CBlockThinIndex*>::iterator mi = mapBlockThinIndex.find(wtx.hashBlock);
        if (mi != mapBlockThinIndex.end())
        {
            pindex = (*mi).second;
            nHeight = pindex->nHeight;
        };
    };

    // Sort order, unrecorded transactions sort to the top
    status.sortKey = strprintf("%010d-%01d-%010u-%03d",
        nHeight,
        (wtx.IsCoinBase() ? 1 : 0),
        wtx.nTimeReceived,
        idx);

    status.countsForBalance = wtx.IsTrusted() && !(wtx.GetBlocksToMaturity() > 0);
    status.depth = wtx.GetDepthInMainChain();
    status.cur_num_blocks = nBestHeight;

    if (!wtx.IsFinal())
    {
        if (wtx.nLockTime < LOCKTIME_THRESHOLD)
        {
            status.status = TransactionStatus::OpenUntilBlock;
            status.open_for = nBestHeight - wtx.nLockTime;
        } else
        {
            status.status = TransactionStatus::OpenUntilDate;
            status.open_for = wtx.nLockTime;
        };
    } else
    if (type == TransactionRecord::Generated)
    {
        // For generated transactions, determine maturity
        if (wtx.GetBlocksToMaturity() > 0)
        {
            status.status = TransactionStatus::Immature;

            if (wtx.IsInMainChain())
            {
                status.matures_in = wtx.GetBlocksToMaturity();

                // Check if the block was requested by anyone
                if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
                    status.status = TransactionStatus::MaturesWarning;
            } else
                status.status = TransactionStatus::NotAccepted;
        } else
            status.status = TransactionStatus::Confirmed;
    } else
    {
        if (status.depth < 0)
            status.status = TransactionStatus::Conflicted;
        else if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
            status.status = TransactionStatus::Offline;
        else if (status.depth == 0)
            status.status = TransactionStatus::Unconfirmed;
        else if (status.depth < RecommendedNumConfirmations)
            status.status = TransactionStatus::Confirming;
        else
            status.status = TransactionStatus::Confirmed;
    }
}

bool TransactionRecord::statusUpdateNeeded()
{
    AssertLockHeld(cs_main);

    return status.cur_num_blocks != nBestHeight;
}

std::string TransactionRecord::getTxID()
{
    return hash.ToString() + strprintf("-%03d", idx);
}

QString TransactionRecord::getTypeLabel()
{
    return getTypeLabel(type);
}
