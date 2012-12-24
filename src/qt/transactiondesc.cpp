#include "transactiondesc.h"

#include "guiutil.h"
#include "bitcoinunits.h"

#include "main.h"
#include "wallet.h"
#include "db.h"
#include "ui_interface.h"

#include <QString>

using namespace std;

QString TransactionDesc::FormatTxStatus(const CWalletTx& wtx)
{
    if (!wtx.IsFinal())
    {
        if (wtx.nLockTime < LOCKTIME_THRESHOLD)
            return tr("Open for %1 blocks").arg(nBestHeight - wtx.nLockTime);
        else
            return tr("Open until %1").arg(GUIUtil::dateTimeStr(wtx.nLockTime));
    }
    else
    {
        int nDepth = wtx.GetDepthInMainChain();
        if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
            return tr("%1/offline?").arg(nDepth);
        else if (nDepth < 6)
            return tr("%1/unconfirmed").arg(nDepth);
        else
            return tr("%1 confirmations").arg(nDepth);
    }
}

QString TransactionDesc::toHTML(CWallet *wallet, CWalletTx &wtx)
{
    QString strHTML;

    {
        LOCK(wallet->cs_wallet);
        strHTML.reserve(4000);
        strHTML += "<html><font face='verdana, arial, helvetica, sans-serif'>";

        int64 nTime = wtx.GetTxTime();
        int64 nCredit = wtx.GetCredit();
        int64 nDebit = wtx.GetDebit();
        int64 nNet = nCredit - nDebit;

        strHTML += tr("<b>Status:</b> ") + FormatTxStatus(wtx);
        int nRequests = wtx.GetRequestCount();
        if (nRequests != -1)
        {
            if (nRequests == 0)
                strHTML += tr(", has not been successfully broadcast yet");
            else if (nRequests == 1)
                strHTML += tr(", broadcast through %1 node").arg(nRequests);
            else
                strHTML += tr(", broadcast through %1 nodes").arg(nRequests);
        }
        strHTML += "<br>";

        strHTML += tr("<b>Date:</b> ") + (nTime ? GUIUtil::dateTimeStr(nTime) : QString("")) + "<br>";

        //
        // From
        //
        if (wtx.IsCoinBase())
        {
            strHTML += tr("<b>Source:</b> Generated<br>");
        }
        else if (!wtx.mapValue["from"].empty())
        {
            // Online transaction
            if (!wtx.mapValue["from"].empty())
                strHTML += tr("<b>From:</b> ") + GUIUtil::HtmlEscape(wtx.mapValue["from"]) + "<br>";
        }
        else
        {
            // Offline transaction
            if (nNet > 0)
            {
                // Credit
                BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                {
                    if (wallet->IsMine(txout))
                    {
                        CBitcoinAddress address;
                        if (ExtractAddress(txout.scriptPubKey, address) && wallet->HaveKey(address))
                        {
                            if (wallet->mapAddressBook.count(address))
                            {
                                strHTML += tr("<b>From:</b> ") + tr("unknown") + "<br>";
                                strHTML += tr("<b>To:</b> ");
                                strHTML += GUIUtil::HtmlEscape(address.ToString());
                                if (!wallet->mapAddressBook[address].empty())
                                    strHTML += tr(" (yours, label: ") + GUIUtil::HtmlEscape(wallet->mapAddressBook[address]) + ")";
                                else
                                    strHTML += tr(" (yours)");
                                strHTML += "<br>";
                            }
                        }
                        break;
                    }
                }
            }
        }

        //
        // To
        //
        string strAddress;
        if (!wtx.mapValue["to"].empty())
        {
            // Online transaction
            strAddress = wtx.mapValue["to"];
            strHTML += tr("<b>To:</b> ");
            if (wallet->mapAddressBook.count(strAddress) && !wallet->mapAddressBook[strAddress].empty())
                strHTML += GUIUtil::HtmlEscape(wallet->mapAddressBook[strAddress]) + " ";
            strHTML += GUIUtil::HtmlEscape(strAddress) + "<br>";
        }

        //
        // Amount
        //
        if (wtx.IsCoinBase() && nCredit == 0)
        {
            //
            // Coinbase
            //
            int64 nUnmatured = 0;
            BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                nUnmatured += wallet->GetCredit(txout);
            strHTML += tr("<b>Credit:</b> ");
            if (wtx.IsInMainChain())
                strHTML += tr("(%1 matures in %2 more blocks)")
                        .arg(BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, nUnmatured))
                        .arg(wtx.GetBlocksToMaturity());
            else
                strHTML += tr("(not accepted)");
            strHTML += "<br>";
        }
        else if (nNet > 0)
        {
            //
            // Credit
            //
            strHTML += tr("<b>Credit:</b> ") + BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, nNet) + "<br>";
        }
        else
        {
            bool fAllFromMe = true;
            BOOST_FOREACH(const CTxIn& txin, wtx.vin)
                fAllFromMe = fAllFromMe && wallet->IsMine(txin);

            bool fAllToMe = true;
            BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                fAllToMe = fAllToMe && wallet->IsMine(txout);

            if (fAllFromMe)
            {
                //
                // Debit
                //
                BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                {
                    if (wallet->IsMine(txout))
                        continue;

                    if (wtx.mapValue["to"].empty())
                    {
                        // Offline transaction
                        CBitcoinAddress address;
                        if (ExtractAddress(txout.scriptPubKey, address))
                        {
                            strHTML += tr("<b>To:</b> ");
                            if (wallet->mapAddressBook.count(address) && !wallet->mapAddressBook[address].empty())
                                strHTML += GUIUtil::HtmlEscape(wallet->mapAddressBook[address]) + " ";
                            strHTML += GUIUtil::HtmlEscape(address.ToString());
                            strHTML += "<br>";
                        }
                    }

                    strHTML += tr("<b>Debit:</b> ") + BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, -txout.nValue) + "<br>";
                }

                if (fAllToMe)
                {
                    // Payment to self
                    int64 nChange = wtx.GetChange();
                    int64 nValue = nCredit - nChange;
                    strHTML += tr("<b>Debit:</b> ") + BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, -nValue) + "<br>";
                    strHTML += tr("<b>Credit:</b> ") + BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, nValue) + "<br>";
                }

                int64 nTxFee = nDebit - wtx.GetValueOut();
                if (nTxFee > 0)
                    strHTML += tr("<b>Transaction fee:</b> ") + BitcoinUnits::formatWithUnit(BitcoinUnits::BTC,-nTxFee) + "<br>";
            }
            else
            {
                //
                // Mixed debit transaction
                //
                BOOST_FOREACH(const CTxIn& txin, wtx.vin)
                    if (wallet->IsMine(txin))
                        strHTML += tr("<b>Debit:</b> ") + BitcoinUnits::formatWithUnit(BitcoinUnits::BTC,-wallet->GetDebit(txin)) + "<br>";
                BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                    if (wallet->IsMine(txout))
                        strHTML += tr("<b>Credit:</b> ") + BitcoinUnits::formatWithUnit(BitcoinUnits::BTC,wallet->GetCredit(txout)) + "<br>";
            }
        }

        strHTML += tr("<b>Net amount:</b> ") + BitcoinUnits::formatWithUnit(BitcoinUnits::BTC,nNet, true) + "<br>";

        //
        // Message
        //
        if (!wtx.mapValue["message"].empty())
            strHTML += QString("<br><b>") + tr("Message:") + "</b><br>" + GUIUtil::HtmlEscape(wtx.mapValue["message"], true) + "<br>";
        if (!wtx.mapValue["comment"].empty())
            strHTML += QString("<br><b>") + tr("Comment:") + "</b><br>" + GUIUtil::HtmlEscape(wtx.mapValue["comment"], true) + "<br>";

        strHTML += QString("<b>") + tr("Transaction ID:") + "</b> " + wtx.GetHash().ToString().c_str() + "<br>";

        if (wtx.IsCoinBase())
            strHTML += QString("<br>") + tr("Generated coins must wait 520 blocks before they can be spent.  When you generated this block, it was broadcast to the network to be added to the block chain.  If it fails to get into the chain, it will change to \"not accepted\" and not be spendable.  This may occasionally happen if another node generates a block within a few seconds of yours.") + "<br>";
        if (wtx.IsCoinStake())
            strHTML += QString("<br>") + tr("Staked coins must wait 520 blocks before they can return to balance and be spent.  When you generated this proof-of-stake block, it was broadcast to the network to be added to the block chain.  If it fails to get into the chain, it will change to \"not accepted\" and not be a valid stake.  This may occasionally happen if another node generates a proof-of-stake block within a few seconds of yours.") + "<br>";

        //
        // Debug view
        //
        if (fDebug)
        {
            strHTML += "<hr><br>Debug information<br><br>";
            BOOST_FOREACH(const CTxIn& txin, wtx.vin)
                if(wallet->IsMine(txin))
                    strHTML += "<b>Debit:</b> " + BitcoinUnits::formatWithUnit(BitcoinUnits::BTC,-wallet->GetDebit(txin)) + "<br>";
            BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                if(wallet->IsMine(txout))
                    strHTML += "<b>Credit:</b> " + BitcoinUnits::formatWithUnit(BitcoinUnits::BTC,wallet->GetCredit(txout)) + "<br>";

            strHTML += "<br><b>Transaction:</b><br>";
            strHTML += GUIUtil::HtmlEscape(wtx.ToString(), true);

            CTxDB txdb("r"); // To fetch source txouts

            strHTML += "<br><b>Inputs:</b>";
            strHTML += "<ul>";

            {
                LOCK(wallet->cs_wallet);
                BOOST_FOREACH(const CTxIn& txin, wtx.vin)
                {
                    COutPoint prevout = txin.prevout;

                    CTransaction prev;
                    if(txdb.ReadDiskTx(prevout.hash, prev))
                    {
                        if (prevout.n < prev.vout.size())
                        {
                            strHTML += "<li>";
                            const CTxOut &vout = prev.vout[prevout.n];
                            CBitcoinAddress address;
                            if (ExtractAddress(vout.scriptPubKey, address))
                            {
                                if (wallet->mapAddressBook.count(address) && !wallet->mapAddressBook[address].empty())
                                    strHTML += GUIUtil::HtmlEscape(wallet->mapAddressBook[address]) + " ";
                                strHTML += QString::fromStdString(address.ToString());
                            }
                            strHTML = strHTML + " Amount=" + BitcoinUnits::formatWithUnit(BitcoinUnits::BTC,vout.nValue);
                            strHTML = strHTML + " IsMine=" + (wallet->IsMine(vout) ? "true" : "false") + "</li>";
                        }
                    }
                }
            }
            strHTML += "</ul>";
        }

        strHTML += "</font></html>";
    }
    return strHTML;
}
