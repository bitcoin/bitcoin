#include "transactiondesc.h"

#include "guiutil.h"
#include "bitcoinunits.h"

#include "main.h"
#include "wallet.h"
#include "db.h"
#include "ui_interface.h"
#include "base58.h"

QString TransactionDesc::FormatTxStatus(const CWalletTx& wtx)
{
    if (!wtx.IsFinal())
    {
        if (wtx.nLockTime < LOCKTIME_THRESHOLD)
            return tr("Open for %n block(s)", "", nBestHeight - wtx.nLockTime);
        else
            return tr("Open until %1").arg(GUIUtil::dateTimeStr(wtx.nLockTime));
    }
    else
    {
        int nDepth = wtx.GetDepthInMainChain();
        if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
            return tr("%1/offline").arg(nDepth);
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

        strHTML += "<b>" + tr("Status") + ":</b> " + FormatTxStatus(wtx);
        int nRequests = wtx.GetRequestCount();
        if (nRequests != -1)
        {
            if (nRequests == 0)
                strHTML += tr(", has not been successfully broadcast yet");
            else if (nRequests > 0)
                strHTML += tr(", broadcast through %n node(s)", "", nRequests);
        }
        strHTML += "<br>";

        strHTML += "<b>" + tr("Date") + ":</b> " + (nTime ? GUIUtil::dateTimeStr(nTime) : "") + "<br>";

        //
        // From
        //
        if (wtx.IsCoinBase())
        {
            strHTML += "<b>" + tr("Source") + ":</b> " + tr("Generated") + "<br>";
        }
        else if (wtx.mapValue.count("from") && !wtx.mapValue["from"].empty())
        {
            // Online transaction
            strHTML += "<b>" + tr("From") + ":</b> " + GUIUtil::HtmlEscape(wtx.mapValue["from"]) + "<br>";
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
                        CTxDestination address;
                        if (ExtractDestination(txout.scriptPubKey, address) && IsMine(*wallet, address))
                        {
                            if (wallet->mapAddressBook.count(address))
                            {
                                strHTML += "<b>" + tr("From") + ":</b> " + tr("unknown") + "<br>";
                                strHTML += "<b>" + tr("To") + ":</b> ";
                                strHTML += GUIUtil::HtmlEscape(CBitcoinAddress(address).ToString());
                                if (!wallet->mapAddressBook[address].empty())
                                    strHTML += " (" + tr("own address") + ", " + tr("label") + ": " + GUIUtil::HtmlEscape(wallet->mapAddressBook[address]) + ")";
                                else
                                    strHTML += " (" + tr("own address") + ")";
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
        if (wtx.mapValue.count("to") && !wtx.mapValue["to"].empty())
        {
            // Online transaction
            std::string strAddress = wtx.mapValue["to"];
            strHTML += "<b>" + tr("To") + ":</b> ";
            CTxDestination dest = CBitcoinAddress(strAddress).Get();
            if (wallet->mapAddressBook.count(dest) && !wallet->mapAddressBook[dest].empty())
                strHTML += GUIUtil::HtmlEscape(wallet->mapAddressBook[dest]) + " ";
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
            strHTML += "<b>" + tr("Credit") + ":</b> ";
            if (wtx.IsInMainChain())
                strHTML += BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, nUnmatured)+ " (" + tr("matures in %n more block(s)", "", wtx.GetBlocksToMaturity()) + ")";
            else
                strHTML += "(" + tr("not accepted") + ")";
            strHTML += "<br>";
        }
        else if (nNet > 0)
        {
            //
            // Credit
            //
            strHTML += "<b>" + tr("Credit") + ":</b> " + BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, nNet) + "<br>";
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

                    if (!wtx.mapValue.count("to") || wtx.mapValue["to"].empty())
                    {
                        // Offline transaction
                        CTxDestination address;
                        if (ExtractDestination(txout.scriptPubKey, address))
                        {
                            strHTML += "<b>" + tr("To") + ":</b> ";
                            if (wallet->mapAddressBook.count(address) && !wallet->mapAddressBook[address].empty())
                                strHTML += GUIUtil::HtmlEscape(wallet->mapAddressBook[address]) + " ";
                            strHTML += GUIUtil::HtmlEscape(CBitcoinAddress(address).ToString());
                            strHTML += "<br>";
                        }
                    }

                    strHTML += "<b>" + tr("Debit") + ":</b> " + BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, -txout.nValue) + "<br>";
                }

                if (fAllToMe)
                {
                    // Payment to self
                    int64 nChange = wtx.GetChange();
                    int64 nValue = nCredit - nChange;
                    strHTML += "<b>" + tr("Debit") + ":</b> " + BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, -nValue) + "<br>";
                    strHTML += "<b>" + tr("Credit") + ":</b> " + BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, nValue) + "<br>";
                }

                int64 nTxFee = nDebit - wtx.GetValueOut();
                if (nTxFee > 0)
                    strHTML += "<b>" + tr("Transaction fee") + ":</b> " + BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, -nTxFee) + "<br>";
            }
            else
            {
                //
                // Mixed debit transaction
                //
                BOOST_FOREACH(const CTxIn& txin, wtx.vin)
                    if (wallet->IsMine(txin))
                        strHTML += "<b>" + tr("Debit") + ":</b> " + BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, -wallet->GetDebit(txin)) + "<br>";
                BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                    if (wallet->IsMine(txout))
                        strHTML += "<b>" + tr("Credit") + ":</b> " + BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, wallet->GetCredit(txout)) + "<br>";
            }
        }

        strHTML += "<b>" + tr("Net amount") + ":</b> " + BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, nNet, true) + "<br>";

        //
        // Message
        //
        if (wtx.mapValue.count("message") && !wtx.mapValue["message"].empty())
            strHTML += "<br><b>" + tr("Message") + ":</b><br>" + GUIUtil::HtmlEscape(wtx.mapValue["message"], true) + "<br>";
        if (wtx.mapValue.count("comment") && !wtx.mapValue["comment"].empty())
            strHTML += "<br><b>" + tr("Comment") + ":</b><br>" + GUIUtil::HtmlEscape(wtx.mapValue["comment"], true) + "<br>";

        strHTML += "<b>" + tr("Transaction ID") + ":</b> " + wtx.GetHash().ToString().c_str() + "<br>";

        if (wtx.IsCoinBase() || wtx.IsCoinStake())
            strHTML += "<br>" + tr("Generated coins must mature 520 blocks before they can be spent. When you generated this block, it was broadcast to the network to be added to the block chain. If it fails to get into the chain, its state will change to \"not accepted\" and it won't be spendable. This may occasionally happen if another node generates a block within a few seconds of yours.") + "<br>";

        //
        // Debug view
        //
        if (fDebug)
        {
            strHTML += "<hr><br>" + tr("Debug information") + "<br><br>";
            BOOST_FOREACH(const CTxIn& txin, wtx.vin)
                if(wallet->IsMine(txin))
                    strHTML += "<b>" + tr("Debit") + ":</b> " + BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, -wallet->GetDebit(txin)) + "<br>";
            BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                if(wallet->IsMine(txout))
                    strHTML += "<b>" + tr("Credit") + ":</b> " + BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, wallet->GetCredit(txout)) + "<br>";

            strHTML += "<br><b>" + tr("Transaction") + ":</b><br>";
            strHTML += GUIUtil::HtmlEscape(wtx.ToString(), true);

            CTxDB txdb("r"); // To fetch source txouts

            strHTML += "<br><b>" + tr("Inputs") + ":</b>";
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
                            CTxDestination address;
                            if (ExtractDestination(vout.scriptPubKey, address))
                            {
                                if (wallet->mapAddressBook.count(address) && !wallet->mapAddressBook[address].empty())
                                    strHTML += GUIUtil::HtmlEscape(wallet->mapAddressBook[address]) + " ";
                                strHTML += QString::fromStdString(CBitcoinAddress(address).ToString());
                            }
                            strHTML = strHTML + " " + tr("Amount") + "=" + BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, vout.nValue);
                            strHTML = strHTML + " IsMine=" + (wallet->IsMine(vout) ? tr("true") : tr("false")) + "</li>";
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
