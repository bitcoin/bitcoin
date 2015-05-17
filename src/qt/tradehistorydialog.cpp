// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tradehistorydialog.h"
#include "ui_tradehistorydialog.h"

#include "guiutil.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "wallet.h"
#include "base58.h"
#include "ui_interface.h"
#include "guiutil.h"

#include <boost/filesystem.hpp>

#include "leveldb/db.h"
#include "leveldb/write_batch.h"

// potentially overzealous includes here
#include "base58.h"
#include "rpcserver.h"
#include "init.h"
#include "util.h"
#include <fstream>
#include <algorithm>
#include <vector>
#include <utility>
#include <string>
#include <boost/assign/list_of.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "leveldb/db.h"
#include "leveldb/write_batch.h"
// end potentially overzealous includes

using namespace json_spirit;
#include "mastercore.h"
using namespace mastercore;

// potentially overzealous using here
using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace leveldb;
// end potentially overzealous using

#include "mastercore_mdex.h"
#include "mastercore_tx.h"
#include "mastercore_sp.h"
#include "mastercore_parse_string.h"
#include "mastercore_rpc.h"
#include "omnicore_qtutils.h"

#include <QDateTime>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextDocument>
#include <QPushButton>

TradeHistoryDialog::TradeHistoryDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::tradeHistoryDialog),
    model(0)
{
    // Setup the UI
    ui->setupUi(this);
    ui->tradeHistoryTable->setColumnCount(7);
    ui->tradeHistoryTable->setHorizontalHeaderItem(1, new QTableWidgetItem(" "));
    ui->tradeHistoryTable->setHorizontalHeaderItem(2, new QTableWidgetItem("Date"));
    ui->tradeHistoryTable->setHorizontalHeaderItem(3, new QTableWidgetItem("Status"));
    ui->tradeHistoryTable->setHorizontalHeaderItem(4, new QTableWidgetItem("Trade Details"));
    ui->tradeHistoryTable->setHorizontalHeaderItem(5, new QTableWidgetItem("Sold"));
    ui->tradeHistoryTable->setHorizontalHeaderItem(6, new QTableWidgetItem("Received"));
    borrowedColumnResizingFixer = new GUIUtil::TableViewLastColumnResizingFixer(ui->tradeHistoryTable,100,100);
    #if QT_VERSION < 0x050000
       ui->tradeHistoryTable->horizontalHeader()->setResizeMode(1, QHeaderView::Fixed);
       ui->tradeHistoryTable->horizontalHeader()->setResizeMode(2, QHeaderView::Interactive);
       ui->tradeHistoryTable->horizontalHeader()->setResizeMode(3, QHeaderView::Interactive);
       ui->tradeHistoryTable->horizontalHeader()->setResizeMode(4, QHeaderView::Interactive);
       ui->tradeHistoryTable->horizontalHeader()->setResizeMode(5, QHeaderView::Interactive);
       ui->tradeHistoryTable->horizontalHeader()->setResizeMode(6, QHeaderView::Interactive);
    #else
       ui->tradeHistoryTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
       ui->tradeHistoryTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);
       ui->tradeHistoryTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive);
       ui->tradeHistoryTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Interactive);
       ui->tradeHistoryTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Interactive);
       ui->tradeHistoryTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Interactive);
    #endif
    ui->tradeHistoryTable->setAlternatingRowColors(true);
    ui->tradeHistoryTable->verticalHeader()->setVisible(false);
    ui->tradeHistoryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tradeHistoryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tradeHistoryTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tradeHistoryTable->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tradeHistoryTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->tradeHistoryTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    Update(); // make sure we're populated before attempting to resize to contents
    ui->tradeHistoryTable->setColumnHidden(0, true);
    ui->tradeHistoryTable->setColumnWidth(1, 23);
    ui->tradeHistoryTable->resizeColumnToContents(2);
    ui->tradeHistoryTable->resizeColumnToContents(3);
    ui->tradeHistoryTable->resizeColumnToContents(5);
    ui->tradeHistoryTable->resizeColumnToContents(6);
    borrowedColumnResizingFixer->stretchColumnWidth(4);
//    ui->orderHistoryTable->setSortingEnabled(true);
//    ui->orderHistoryTable->horizontalHeader()->setSortIndicator(2, Qt::DescendingOrder);
    QAction *copyTxIDAction = new QAction(tr("Copy transaction ID"), this);
    QAction *showDetailsAction = new QAction(tr("Show trade details"), this);
    contextMenu = new QMenu();
    contextMenu->addAction(copyTxIDAction);
    contextMenu->addAction(showDetailsAction);
    connect(ui->tradeHistoryTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));
    connect(ui->tradeHistoryTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(showDetails()));
    connect(copyTxIDAction, SIGNAL(triggered()), this, SLOT(copyTxID()));
    connect(showDetailsAction, SIGNAL(triggered()), this, SLOT(showDetails()));
}

void TradeHistoryDialog::Update()
{
    //pending orders
    int rowcount = 0;

    // handle pending transactions first
    for(PendingMap::iterator it = my_pending.begin(); it != my_pending.end(); ++it)
    {
        CMPPending *p_pending = &(it->second);
        uint256 txid = it->first;
        string txidStr = txid.GetHex();

        string senderAddress = p_pending->src;
        uint64_t propertyId = p_pending->prop;
        bool divisible = isPropertyDivisible(propertyId);
        string displayAmount;
        int64_t amount = p_pending->amount;
        string displayToken;
        string displayValid;
        string displayAddress = senderAddress;
        int64_t type = p_pending->type;
        if (type == 21)
        {
            if (divisible) { displayAmount = FormatDivisibleShortMP(amount); } else { displayAmount = FormatIndivisibleMP(amount); }
            QString txTimeStr = "Unconfirmed";
            string statusText = "Pending";
            string displayText = "Sell ";
            string displayInToken;
            string displayOutToken;
            if(divisible) { displayText += FormatDivisibleShortMP(amount); } else { displayText += FormatIndivisibleMP(amount); }
            if(propertyId < 3)
            {
                if(propertyId == 1) { displayText += " MSC"; displayOutToken = " MSC"; }
                if(propertyId == 2) { displayText += " TMSC"; displayOutToken = " TMSC"; }
            }
            else
            {
                string s = static_cast<ostringstream*>( &(ostringstream() << propertyId) )->str();
                displayText += " SPT#" + s + "";
                displayOutToken = " SPT#" + s;
            }
            displayText += " (awaiting confirmation)";
            string displayIn = "---";
            string displayOut = "---";
            //icon
            QIcon ic = QIcon(":/icons/transaction_0");
            // add to history
            ui->tradeHistoryTable->setRowCount(rowcount+1);
            QTableWidgetItem *dateCell = new QTableWidgetItem(txTimeStr);
            QTableWidgetItem *statusCell = new QTableWidgetItem(QString::fromStdString(statusText));
            QTableWidgetItem *infoCell = new QTableWidgetItem(QString::fromStdString(displayText));
            QTableWidgetItem *amountOutCell = new QTableWidgetItem(QString::fromStdString(displayOut));
            QTableWidgetItem *amountInCell = new QTableWidgetItem(QString::fromStdString(displayIn));
            QTableWidgetItem *iconCell = new QTableWidgetItem;
            QTableWidgetItem *txidCell = new QTableWidgetItem(QString::fromStdString(txidStr));
            iconCell->setIcon(ic);
            amountOutCell->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
            amountOutCell->setForeground(QColor("#EE0000"));
            amountInCell->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
            amountInCell->setForeground(QColor("#00AA00"));
            if (rowcount % 2)
            {
                dateCell->setBackground(QColor("#F0F0F0"));
                statusCell->setBackground(QColor("#F0F0F0"));
                infoCell->setBackground(QColor("#F0F0F0"));
                amountOutCell->setBackground(QColor("#F0F0F0"));
                amountInCell->setBackground(QColor("#F0F0F0"));
                iconCell->setBackground(QColor("#F0F0F0"));
                txidCell->setBackground(QColor("#F0F0F0"));
            }
            amountInCell->setForeground(QColor("#000000"));
            amountOutCell->setForeground(QColor("#000000"));

            ui->tradeHistoryTable->setItem(rowcount, 0, txidCell);
            ui->tradeHistoryTable->setItem(rowcount, 1, iconCell);
            ui->tradeHistoryTable->setItem(rowcount, 2, dateCell);
            ui->tradeHistoryTable->setItem(rowcount, 3, statusCell);
            ui->tradeHistoryTable->setItem(rowcount, 4, infoCell);
            ui->tradeHistoryTable->setItem(rowcount, 5, amountOutCell);
            ui->tradeHistoryTable->setItem(rowcount, 6, amountInCell);
            rowcount += 1;
        }
    }


    //wallet orders
    CWallet *wallet = pwalletMain;
    string sAddress = "";
    int64_t nStartBlock = 0;
    int64_t nEndBlock = 999999;

    // rewrite to use original listtransactions methodology from core
    LOCK(wallet->cs_wallet);
    std::list<CAccountingEntry> acentries;
    CWallet::TxItems txOrdered = pwalletMain->OrderedTxItems(acentries, "*");

    // iterate backwards
    for (CWallet::TxItems::reverse_iterator it = txOrdered.rbegin(); it != txOrdered.rend(); ++it)
    {
        CWalletTx *const pwtx = (*it).second.first;
        if (pwtx != 0)
        {
            uint256 hash = pwtx->GetHash();
            CTransaction wtx;
            uint256 blockHash = 0;
            if (!GetTransaction(hash, wtx, blockHash, true)) continue;
            // get the time of the tx
            int64_t nTime = pwtx->GetTxTime();
            // get the height of the transaction and check it's within the chosen parameters
            blockHash = pwtx->hashBlock;
            if ((0 == blockHash) || (NULL == mapBlockIndex[blockHash])) continue;
            CBlockIndex* pBlockIndex = mapBlockIndex[blockHash];
            if (NULL == pBlockIndex) continue;
            int blockHeight = pBlockIndex->nHeight;
            if ((blockHeight < nStartBlock) || (blockHeight > nEndBlock)) continue; // ignore it if not within our range
            // check if the transaction exists in txlist, and if so is it correct type (21)
            if (p_txlistdb->exists(hash))
            {
                // get type from levelDB
                string strValue;
                if (!p_txlistdb->getTX(hash, strValue)) continue;
                std::vector<std::string> vstr;
                boost::split(vstr, strValue, boost::is_any_of(":"), token_compress_on);
                if (4 <= vstr.size())
                {
                    // if tx21, get the details for the list
                    if(21 == atoi(vstr[2]))
                    {
                        string statusText;
                        unsigned int propertyIdForSale = 0;
                        unsigned int propertyIdDesired = 0;
                        uint64_t amountForSale = 0;
                        uint64_t amountDesired = 0;
                        string address;
                        bool divisibleForSale = false;
                        bool divisibleDesired = false;
                        Array tradeArray;
                        uint64_t totalBought = 0;
                        uint64_t totalSold = 0;
                        bool orderOpen = false;
                        bool valid = false;

                        CMPMetaDEx temp_metadexoffer;
                        CMPTransaction mp_obj;
                        int parseRC = parseTransaction(true, wtx, blockHeight, 0, &mp_obj);
                        if (0 <= parseRC) //negative RC means no MP content/badly encoded TX, we shouldn't see this if TX in levelDB but check for sanity
                        {
                            if (0<=mp_obj.step1())
                            {
                                //MPTxType = mp_obj.getTypeString();
                                //MPTxTypeInt = mp_obj.getType();
                                address = mp_obj.getSender();
                                //if (!filterAddress.empty()) if ((senderAddress != filterAddress) && (refAddress != filterAddress)) return -1; // return negative rc if filtering & no match

                                int tmpblock=0;
                                uint32_t tmptype=0;
                                uint64_t amountNew=0;
                                valid=getValidMPTX(hash, &tmpblock, &tmptype, &amountNew);

                                if (0 == mp_obj.step2_Value())
                                {
                                    propertyIdForSale = mp_obj.getProperty();
                                    amountForSale = mp_obj.getAmount();
                                    divisibleForSale = isPropertyDivisible(propertyIdForSale);
                                    if (0 <= mp_obj.interpretPacket(NULL,&temp_metadexoffer))
                                    {
                                        propertyIdDesired = temp_metadexoffer.getDesProperty();
                                        divisibleDesired = isPropertyDivisible(propertyIdDesired);
                                        amountDesired = temp_metadexoffer.getAmountDesired();
                                        //mdex_action = temp_metadexoffer.getAction();
                                        t_tradelistdb->getMatchingTrades(hash, propertyIdForSale, &tradeArray, &totalSold, &totalBought);

                                        // status - is order cancelled/closed-filled/open/open-partialfilled?
                                        // is the sell offer still open - need more efficient way to do this
                                        for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it)
                                        {
                                            if (my_it->first == propertyIdForSale) //at minimum only go deeper if it's the right property id
                                            {
                                                md_PricesMap & prices = my_it->second;
                                                for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it)
                                                {
                                                    md_Set & indexes = (it->second);
                                                    for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it)
                                                    {
                                                        CMPMetaDEx obj = *it;
                                                        if( obj.getHash().GetHex() == hash.GetHex() ) orderOpen = true;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // work out status
                        bool partialFilled = false;
                        bool filled = false;
                        if(totalSold>0) partialFilled = true;
                        if(totalSold>=amountForSale) filled = true;
                        statusText = "Unknown";
                        if((!orderOpen) && (!partialFilled)) statusText = "Cancelled";
                        if((!orderOpen) && (partialFilled)) statusText = "Part Cancel";
                        if((!orderOpen) && (filled)) statusText = "Filled";
                        if((orderOpen) && (!partialFilled)) statusText = "Open";
                        if((orderOpen) && (partialFilled)) statusText = "Part Filled";

                        // add to list
                        string displayText = "Sell ";
                        string displayIn = "";
                        string displayOut = "-";
                        string displayInToken;
                        string displayOutToken;

                        if(divisibleForSale) { displayText += FormatDivisibleShortMP(amountForSale); } else { displayText += FormatIndivisibleMP(amountForSale); }
                        if(propertyIdForSale < 3)
                        {
                            if(propertyIdForSale == 1) { displayText += " MSC for "; displayOutToken = " MSC"; }
                            if(propertyIdForSale == 2) { displayText += " TMSC for "; displayOutToken = " TMSC"; }
                        }
                        else
                        {
                            string s = static_cast<ostringstream*>( &(ostringstream() << propertyIdForSale) )->str();
                            displayText += " SPT#" + s + " for ";
                            displayOutToken = " SPT#" + s;
                        }
                        if(divisibleDesired) { displayText += FormatDivisibleShortMP(amountDesired); } else { displayText += FormatIndivisibleMP(amountDesired); }
                        if(propertyIdDesired < 3)
                        {
                            if(propertyIdDesired == 1) { displayText += " MSC"; displayInToken = " MSC"; }
                            if(propertyIdDesired == 2) { displayText += " TMSC"; displayInToken = " TMSC"; }
                        }
                        else
                        {
                            string s = static_cast<ostringstream*>( &(ostringstream() << propertyIdDesired) )->str();
                            displayText += " SPT#" + s;
                            displayInToken = " SPT#" + s;
                        }
                        if(divisibleDesired) { displayIn += FormatDivisibleShortMP(totalBought); } else { displayIn += FormatIndivisibleMP(totalBought); }
                        if(divisibleForSale) { displayOut += FormatDivisibleShortMP(totalSold); } else { displayOut += FormatIndivisibleMP(totalSold); }
                        if(totalBought == 0) displayIn = "0";
                        if(totalSold == 0) displayOut = "0";
                        displayIn += displayInToken;
                        displayOut += displayOutToken;
                        QDateTime txTime;
                        txTime.setTime_t(nTime);
                        QString txTimeStr = txTime.toString(Qt::SystemLocaleShortDate);

                        //icon
                        QIcon ic = QIcon(":/icons/transaction_0");
                        if(statusText == "Cancelled") ic =QIcon(":/icons/meta_cancelled");
                        if(statusText == "Part Cancel") ic = QIcon(":/icons/meta_partialclosed");
                        if(statusText == "Filled") ic = QIcon(":/icons/meta_filled");
                        if(statusText == "Open") ic = QIcon(":/icons/meta_open");
                        if(statusText == "Part Filled") ic = QIcon(":/icons/meta_partial");
                        if(!valid) ic = QIcon(":/icons/transaction_invalid");
                        // add to order history
                        ui->tradeHistoryTable->setRowCount(rowcount+1);
                        QTableWidgetItem *dateCell = new QTableWidgetItem(txTimeStr);
                        QTableWidgetItem *statusCell = new QTableWidgetItem(QString::fromStdString(statusText));
                        QTableWidgetItem *infoCell = new QTableWidgetItem(QString::fromStdString(displayText));
                        QTableWidgetItem *amountOutCell = new QTableWidgetItem(QString::fromStdString(displayOut));
                        QTableWidgetItem *amountInCell = new QTableWidgetItem(QString::fromStdString(displayIn));
                        QTableWidgetItem *iconCell = new QTableWidgetItem;
                        QTableWidgetItem *txidCell = new QTableWidgetItem(QString::fromStdString(hash.GetHex()));
                        iconCell->setIcon(ic);
                        //addressCell->setTextAlignment(Qt::AlignLeft + Qt::AlignVCenter);
                        //addressCell->setForeground(QColor("#707070"));
                        amountOutCell->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
                        amountOutCell->setForeground(QColor("#EE0000"));
                        amountInCell->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
                        amountInCell->setForeground(QColor("#00AA00"));
                        if (rowcount % 2)
                        {
                            dateCell->setBackground(QColor("#F0F0F0"));
                            statusCell->setBackground(QColor("#F0F0F0"));
                            infoCell->setBackground(QColor("#F0F0F0"));
                            amountOutCell->setBackground(QColor("#F0F0F0"));
                            amountInCell->setBackground(QColor("#F0F0F0"));
                            iconCell->setBackground(QColor("#F0F0F0"));
                            txidCell->setBackground(QColor("#F0F0F0"));
                        }
                        if((!orderOpen) && (filled)) //make filled orders background
                        {
                            dateCell->setForeground(QColor("#707070"));
                            statusCell->setForeground(QColor("#707070"));
                            infoCell->setForeground(QColor("#707070"));
                            amountOutCell->setForeground(QColor("#993333"));
                            amountInCell->setForeground(QColor("#006600"));
                        }
                        if(displayIn.substr(0,2) == "0 ") amountInCell->setForeground(QColor("#000000"));
                        if(displayOut.substr(0,2) == "0 ") amountOutCell->setForeground(QColor("#000000"));

                        ui->tradeHistoryTable->setItem(rowcount, 0, txidCell);
                        ui->tradeHistoryTable->setItem(rowcount, 1, iconCell);
                        ui->tradeHistoryTable->setItem(rowcount, 2, dateCell);
                        ui->tradeHistoryTable->setItem(rowcount, 3, statusCell);
                        ui->tradeHistoryTable->setItem(rowcount, 4, infoCell);
                        ui->tradeHistoryTable->setItem(rowcount, 5, amountOutCell);
                        ui->tradeHistoryTable->setItem(rowcount, 6, amountInCell);
                        rowcount += 1;
                    }
                }
            }
        }
    }
}

void TradeHistoryDialog::setModel(WalletModel *model)
{
    this->model = model;
    connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(Update()));
}

void TradeHistoryDialog::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tradeHistoryTable->indexAt(point);
    if(index.isValid()) contextMenu->exec(QCursor::pos());
}

void TradeHistoryDialog::copyTxID()
{
    GUIUtil::setClipboard(ui->tradeHistoryTable->item(ui->tradeHistoryTable->currentRow(),6)->text());
}

/* Opens a dialog containing the details of the selected trade and any associated matches
 */
void TradeHistoryDialog::showDetails()
{
    Object txobj;
    uint256 txid;
    txid.SetHex(ui->tradeHistoryTable->item(ui->tradeHistoryTable->currentRow(),0)->text().toStdString());
    std::string strTXText;

    // first of all check if the TX is a pending tx, if so grab details from pending map
    PendingMap::iterator it = my_pending.find(txid);
    if (it != my_pending.end()) {
        CMPPending *p_pending = &(it->second);
        strTXText = "*** THIS TRANSACTION IS UNCONFIRMED ***\n" + p_pending->desc;
    } else {
        int pop = populateRPCTransactionObject(txid, &txobj, "");
        if (0<=pop) {
            Object tradeobj;
            CMPMetaDEx temp_metadexoffer;
            string senderAddress;
            unsigned int propertyId = 0;
            CTransaction wtx;
            uint256 blockHash = 0;
            if (!GetTransaction(txid, wtx, blockHash, true)) { return; }
            CMPTransaction mp_obj;
            int parseRC = parseTransaction(true, wtx, 0, 0, &mp_obj);
            if (0 <= parseRC) { //negative RC means no MP content/badly encoded TX, we shouldn't see this if TX in levelDB but check for sanity
                if (0<=mp_obj.step1()) {
                    senderAddress = mp_obj.getSender();
                    if (0 == mp_obj.step2_Value()) propertyId = mp_obj.getProperty();
                }
            }
            uint64_t amountForSale = mp_obj.getAmount();

            // obtain action byte
            int actionByte = 0;
            if (0 <= mp_obj.interpretPacket(NULL,&temp_metadexoffer)) { actionByte = (int)temp_metadexoffer.getAction(); }

            // obtain an array of matched trades (action 1) or array of cancelled trades (action 2/3/4)
            Array tradeArray, cancelArray;
            uint64_t totalBought = 0, totalSold = 0;
            if (actionByte == 1) {
                t_tradelistdb->getMatchingTrades(txid, propertyId, &tradeArray, &totalSold, &totalBought);
            } else {
                int numberOfCancels = p_txlistdb->getNumberOfMetaDExCancels(txid);
                if (numberOfCancels > 0) {
                    for(int refNumber = 1; refNumber <= numberOfCancels; refNumber++) {
                        Object cancelTx;
                        string strValue = p_txlistdb->getKeyValue(txid.ToString() + "-C" + static_cast<ostringstream*>( &(ostringstream() << refNumber) )->str() );
                        if (!strValue.empty()) {
                            std::vector<std::string> vstr;
                            boost::split(vstr, strValue, boost::is_any_of(":"), token_compress_on);
                            if (3 <= vstr.size()) {
                                uint64_t propId = boost::lexical_cast<uint64_t>(vstr[1]);
                                cancelTx.push_back(Pair("txid", vstr[0]));
                                cancelTx.push_back(Pair("propertyid", propId));
                                cancelTx.push_back(Pair("amountunreserved", FormatMP(propId, boost::lexical_cast<uint64_t>(vstr[2]))));
                                cancelArray.push_back(cancelTx);
                            }
                        }
                    }
                }
            }

            // obtain the status of the trade
            bool orderOpen = isMetaDExOfferActive(txid, propertyId);
            bool partialFilled = false, filled = false;
            string statusText = "unknown";
            if (totalSold>0) partialFilled = true;
            if (totalSold>=amountForSale) filled = true;
            if (orderOpen) {
                if (!partialFilled) { statusText = "open"; } else { statusText = "open part filled"; }
            } else {
                if (!partialFilled) { statusText = "cancelled"; } else { statusText = "cancelled part filled"; }
                if (filled) statusText = "filled";
            }
            if (actionByte == 1) { txobj.push_back(Pair("status", statusText)); } else { txobj.push_back(Pair("status", "N/A")); }

            // add the appropriate array based on action byte
            if(actionByte == 1) {
                txobj.push_back(Pair("matches", tradeArray));
                if((statusText == "cancelled") || (statusText == "cancelled part filled")) txobj.push_back(Pair("canceltxid", p_txlistdb->findMetaDExCancel(txid).GetHex()));
            } else {
                txobj.push_back(Pair("cancelledtransactions", cancelArray));
            }

            strTXText = write_string(Value(txobj), true);
        }
    }

    if (!strTXText.empty()) {
        PopulateSimpleDialog(strTXText, "Trade Information", "Trade Information");
    }
}

void TradeHistoryDialog::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    borrowedColumnResizingFixer->stretchColumnWidth(4);
}

