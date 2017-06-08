// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tradehistorydialog.h"
#include "ui_tradehistorydialog.h"

#include "omnicore_qtutils.h"

#include "guiutil.h"
#include "ui_interface.h"
#include "walletmodel.h"
#include "clientmodel.h"
#include "platformstyle.h"

#include "omnicore/fetchwallettx.h"
#include "omnicore/mdex.h"
#include "omnicore/omnicore.h"
#include "omnicore/pending.h"
#include "omnicore/rpc.h"
#include "omnicore/rpctxobject.h"
#include "omnicore/sp.h"
#include "omnicore/tx.h"
#include "omnicore/utilsbitcoin.h"
#include "omnicore/walletcache.h"
#include "omnicore/wallettxs.h"

#include "amount.h"
#include "init.h"
#include "main.h"
#include "primitives/transaction.h"
#include "sync.h"
#include "txdb.h"
#include "uint256.h"
#include "wallet/wallet.h"

#include <boost/algorithm/string.hpp>

#include <stdint.h>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <QAction>
#include <QDateTime>
#include <QDialog>
#include <QHeaderView>
#include <QIcon>
#include <QMenu>
#include <QModelIndex>
#include <QPoint>
#include <QResizeEvent>
#include <QSortFilterProxyModel>
#include <QString>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QWidget>

using std::ostringstream;
using std::string;

using namespace mastercore;

bool hideInactiveTrades = false;

TradeHistoryDialog::TradeHistoryDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::tradeHistoryDialog),
    clientModel(0),
    walletModel(0)
{
    // Setup the UI
    ui->setupUi(this);
    ui->tradeHistoryTable->setColumnCount(8);
    // Note there are two hidden fields in tradeHistoryTable: 0=txid, 1=lastUpdateBlock
    ui->tradeHistoryTable->setHorizontalHeaderItem(2, new QTableWidgetItem(" "));
    ui->tradeHistoryTable->setHorizontalHeaderItem(3, new QTableWidgetItem("Date"));
    ui->tradeHistoryTable->setHorizontalHeaderItem(4, new QTableWidgetItem("Status"));
    ui->tradeHistoryTable->setHorizontalHeaderItem(5, new QTableWidgetItem("Trade Details"));
    ui->tradeHistoryTable->setHorizontalHeaderItem(6, new QTableWidgetItem("Sold"));
    ui->tradeHistoryTable->setHorizontalHeaderItem(7, new QTableWidgetItem("Received"));
    borrowedColumnResizingFixer = new GUIUtil::TableViewLastColumnResizingFixer(ui->tradeHistoryTable, 100, 100, this);
    #if QT_VERSION < 0x050000
       ui->tradeHistoryTable->horizontalHeader()->setResizeMode(2, QHeaderView::Fixed);
       ui->tradeHistoryTable->horizontalHeader()->setResizeMode(3, QHeaderView::Interactive);
       ui->tradeHistoryTable->horizontalHeader()->setResizeMode(4, QHeaderView::Interactive);
       ui->tradeHistoryTable->horizontalHeader()->setResizeMode(5, QHeaderView::Interactive);
       ui->tradeHistoryTable->horizontalHeader()->setResizeMode(6, QHeaderView::Interactive);
       ui->tradeHistoryTable->horizontalHeader()->setResizeMode(7, QHeaderView::Interactive);
    #else
       ui->tradeHistoryTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
       ui->tradeHistoryTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive);
       ui->tradeHistoryTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Interactive);
       ui->tradeHistoryTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Interactive);
       ui->tradeHistoryTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Interactive);
       ui->tradeHistoryTable->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Interactive);
    #endif
    ui->tradeHistoryTable->setAlternatingRowColors(true);
    ui->tradeHistoryTable->verticalHeader()->setVisible(false);
    ui->tradeHistoryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tradeHistoryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tradeHistoryTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tradeHistoryTable->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tradeHistoryTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->tradeHistoryTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    UpdateTradeHistoryTable(); // make sure we're populated before attempting to resize to contents
    ui->tradeHistoryTable->setColumnHidden(0, true);
    ui->tradeHistoryTable->setColumnHidden(1, true);
    ui->tradeHistoryTable->setColumnWidth(2, 23);
    ui->tradeHistoryTable->resizeColumnToContents(3);
    ui->tradeHistoryTable->resizeColumnToContents(4);
    ui->tradeHistoryTable->resizeColumnToContents(6);
    ui->tradeHistoryTable->resizeColumnToContents(7);
    borrowedColumnResizingFixer->stretchColumnWidth(5);
    ui->tradeHistoryTable->setSortingEnabled(true);
    ui->tradeHistoryTable->horizontalHeader()->setSortIndicator(3, Qt::DescendingOrder);
    QAction *copyTxIDAction = new QAction(tr("Copy transaction ID"), this);
    QAction *showDetailsAction = new QAction(tr("Show trade details"), this);
    contextMenu = new QMenu();
    contextMenu->addAction(copyTxIDAction);
    contextMenu->addAction(showDetailsAction);
    connect(ui->tradeHistoryTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));
    connect(ui->tradeHistoryTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(showDetails()));
    connect(ui->hideInactiveTrades, SIGNAL(stateChanged(int)), this, SLOT(RepopulateTradeHistoryTable(int)));
    connect(copyTxIDAction, SIGNAL(triggered()), this, SLOT(copyTxID()));
    connect(showDetailsAction, SIGNAL(triggered()), this, SLOT(showDetails()));
}

TradeHistoryDialog::~TradeHistoryDialog()
{
    delete ui;
}

void TradeHistoryDialog::ReinitTradeHistoryTable()
{
    ui->tradeHistoryTable->setRowCount(0);
    tradeHistoryMap.clear();
    UpdateTradeHistoryTable();
}

// Repopulate tradeHistoryTable (eg in the case that we are hiding or revealing trades)
void TradeHistoryDialog::RepopulateTradeHistoryTable(int hide)
{
    ui->tradeHistoryTable->setRowCount(0);
    if (hide) {
        hideInactiveTrades = true;
    } else {
        hideInactiveTrades = false;
    }
    UpdateTradeHistoryTable(true);
}

// The main function to update the UI tradeHistoryTable
void TradeHistoryDialog::UpdateTradeHistoryTable(bool forceUpdate)
{
    // Populate tradeHistoryMap
    int newTXCount = PopulateTradeHistoryMap();

    // Process any new transactions that were added to the map
    if (forceUpdate || newTXCount > 0) {
        ui->tradeHistoryTable->setSortingEnabled(false); // disable sorting while we update the table
        QAbstractItemModel* tradeHistoryAbstractModel = ui->tradeHistoryTable->model();
        int chainHeight = GetHeight();

        // Loop through tradeHistoryMap and search tradeHistoryTable for the transaction, adding it if not already there
        for (TradeHistoryMap::iterator it = tradeHistoryMap.begin(); it != tradeHistoryMap.end(); ++it) {
            uint256 txid = it->first;
            TradeHistoryObject objTH = it->second;

            // ignore this trade if it's not active and we've elected to hide inactive trades
            if (hideInactiveTrades && (objTH.status == "Cancelled" || objTH.status == "Filled" || objTH.status == "Part Cancel" || !objTH.valid)) continue;

            // search tradeHistoryTable for an existing row & skip if already there
            QSortFilterProxyModel tradeHistoryProxy;
            tradeHistoryProxy.setSourceModel(tradeHistoryAbstractModel);
            tradeHistoryProxy.setFilterKeyColumn(0);
            tradeHistoryProxy.setFilterFixedString(QString::fromStdString(txid.GetHex()));
            QModelIndex rowIndex = tradeHistoryProxy.mapToSource(tradeHistoryProxy.index(0,0));
            if (rowIndex.isValid()) continue;

            // new entry is required, append a new row (sorting will take care of ordering)
            int newRow = ui->tradeHistoryTable->rowCount();
            ui->tradeHistoryTable->insertRow(newRow);

            // Create the cells to be added to the new row and setup their formatting
            QTableWidgetItem *dateCell = new QTableWidgetItem;
            QDateTime txTime;
            if (objTH.blockHeight > 0) {
                LOCK(cs_main);
                CBlockIndex* pBlkIdx = chainActive[objTH.blockHeight];
                if (NULL != pBlkIdx) txTime.setTime_t(pBlkIdx->GetBlockTime());
                dateCell->setData(Qt::DisplayRole, txTime);
            } else {
                dateCell->setData(Qt::DisplayRole, QString::fromStdString("Unconfirmed"));
            }
            QTableWidgetItem *lastUpdateBlockCell = new QTableWidgetItem(QString::fromStdString(FormatIndivisibleMP(chainHeight)));
            QTableWidgetItem *statusCell = new QTableWidgetItem(QString::fromStdString(objTH.status));
            QTableWidgetItem *infoCell = new QTableWidgetItem(QString::fromStdString(objTH.info));
            QTableWidgetItem *amountOutCell = new QTableWidgetItem(QString::fromStdString(objTH.amountOut));
            QTableWidgetItem *amountInCell = new QTableWidgetItem(QString::fromStdString(objTH.amountIn));
            QTableWidgetItem *txidCell = new QTableWidgetItem(QString::fromStdString(txid.GetHex()));
            QTableWidgetItem *iconCell = new QTableWidgetItem;
            QIcon ic = QIcon(":/icons/omni_meta_pending");
            if (objTH.status == "Cancelled") ic =QIcon(":/icons/omni_meta_cancelled");
            if (objTH.status == "Part Cancel") ic = QIcon(":/icons/omni_meta_partcancelled");
            if (objTH.status == "Filled") ic = QIcon(":/icons/omni_meta_filled");
            if (objTH.status == "Open") ic = QIcon(":/icons/omni_meta_open");
            if (objTH.status == "Part Filled") ic = QIcon(":/icons/omni_meta_partfilled");
            if (!objTH.valid) {
                ic = QIcon(":/icons/transaction_conflicted");
                objTH.status = "Invalid";
            }
//            ic = platformStyle->SingleColorIcon(ic);
            iconCell->setIcon(ic);
            amountOutCell->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
            amountOutCell->setForeground(QColor("#EE0000"));
            amountInCell->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
            amountInCell->setForeground(QColor("#00AA00"));
            if (objTH.status == "Cancelled" || objTH.status == "Filled" || objTH.status == "Part Cancel" || !objTH.valid) {
                // dull the colors for non-active trades
                dateCell->setForeground(QColor("#707070"));
                statusCell->setForeground(QColor("#707070"));
                infoCell->setForeground(QColor("#707070"));
                amountOutCell->setForeground(QColor("#993333"));
                amountInCell->setForeground(QColor("#006600"));
            }
            if(objTH.amountIn.substr(0,2) == "0 " || objTH.amountIn == "---" ) amountInCell->setForeground(QColor("#000000"));
            if(objTH.amountOut.substr(0,2) == "0 " || objTH.amountOut == "---" ) amountOutCell->setForeground(QColor("#000000"));

            // Set the cells in the new row accordingly
            ui->tradeHistoryTable->setItem(newRow, 0, txidCell);
            ui->tradeHistoryTable->setItem(newRow, 1, lastUpdateBlockCell);
            ui->tradeHistoryTable->setItem(newRow, 2, iconCell);
            ui->tradeHistoryTable->setItem(newRow, 3, dateCell);
            ui->tradeHistoryTable->setItem(newRow, 4, statusCell);
            ui->tradeHistoryTable->setItem(newRow, 5, infoCell);
            ui->tradeHistoryTable->setItem(newRow, 6, amountOutCell);
            ui->tradeHistoryTable->setItem(newRow, 7, amountInCell);
        }
        ui->tradeHistoryTable->setSortingEnabled(true); // re-enable sorting
    }

    // Update any existing rows that may have changed data
    UpdateData();
}

// Used to cache trades so we don't need to reparse all our transactions on every update
int TradeHistoryDialog::PopulateTradeHistoryMap()
{
    // TODO: locks may not be needed here -- looks like wallet lock can be removed
    if (NULL == pwalletMain) return -1;
    TRY_LOCK(cs_main,lckMain);
    if (!lckMain) return -1;
    TRY_LOCK(pwalletMain->cs_wallet, lckWallet);
    if (!lckWallet) return -1;

    int64_t nProcessed = 0; // number of new entries, forms return code

    // ### START PENDING TRANSACTIONS PROCESSING ###
    {
        LOCK(cs_pending);

        for (PendingMap::iterator it = my_pending.begin(); it != my_pending.end(); ++it) {
            uint256 txid = it->first;

            // check historyMap, if this tx exists don't waste resources doing anymore work on it
            TradeHistoryMap::iterator hIter = tradeHistoryMap.find(txid);
            if (hIter != tradeHistoryMap.end()) continue;

            // grab pending object, extract details and skip if not a metadex trade
            CMPPending *p_pending = &(it->second);
            if (p_pending->type != MSC_TYPE_METADEX_TRADE) continue;
            uint32_t propertyId = p_pending->prop;
            int64_t amount = p_pending->amount;

            // create a TradeHistoryObject and populate it
            TradeHistoryObject objTH;
            objTH.blockHeight = 0;
            objTH.valid = true; // all pending transactions are assumed to be valid
            objTH.propertyIdForSale = propertyId;
            objTH.propertyIdDesired = 0; // unknown at this stage & not needed for pending
            objTH.amountForSale = amount;
            objTH.status = "Pending";
            objTH.amountIn = "---";
            objTH.amountOut = "---";
            objTH.info = "Sell ";
            if (isPropertyDivisible(propertyId)) {
                objTH.info += FormatDivisibleShortMP(amount) + getTokenLabel(propertyId) + " (awaiting confirmation)";
            } else {
                objTH.info += FormatIndivisibleMP(amount) + getTokenLabel(propertyId) + " (awaiting confirmation)";
            }

            // add the new TradeHistoryObject to the map
            tradeHistoryMap.insert(std::make_pair(txid, objTH));
            nProcessed += 1;
        }
    }
    // ### END PENDING TRANSACTIONS PROCESSING ###

    // ### START WALLET TRANSACTIONS PROCESSING ###
    // obtain a sorted list of Omni layer wallet transactions (including STO receipts and pending) - default last 65535
    std::map<std::string,uint256> walletTransactions = FetchWalletOmniTransactions(GetArg("-omniuiwalletscope", 65535L));

    // reverse iterate over (now ordered) transactions and populate history map for each one
    for (std::map<std::string,uint256>::reverse_iterator it = walletTransactions.rbegin(); it != walletTransactions.rend(); it++) {
        uint256 hash = it->second;

        // use levelDB to perform a fast check on whether it's a bitcoin or Omni tx and whether it's a trade
        std::string tempStrValue;
        {
            LOCK(cs_tally);
            if (!p_txlistdb->getTX(hash, tempStrValue)) continue;
        }
        std::vector<std::string> vstr;
        boost::split(vstr, tempStrValue, boost::is_any_of(":"), boost::token_compress_on);
        if (vstr.size() > 2) {
            if (atoi(vstr[2]) != MSC_TYPE_METADEX_TRADE) continue;
        }

        // check historyMap, if this tx exists don't waste resources doing anymore work on it
        TradeHistoryMap::iterator hIter = tradeHistoryMap.find(hash);
        if (hIter != tradeHistoryMap.end()) {
            // the tx is in historyMap, check if it has a blockheight of 0 (means a pending has confirmed & we should delete pending tx and add it again via parsing)
            TradeHistoryObject *objTH = &(hIter->second);
            if (objTH->blockHeight != 0) {
                continue;
            } else {
                tradeHistoryMap.erase(hIter);
                ui->tradeHistoryTable->setSortingEnabled(false); // disable sorting temporarily while we update the table (leaving enabled gives unexpected results)
                QAbstractItemModel* tradeHistoryAbstractModel = ui->tradeHistoryTable->model();
                QSortFilterProxyModel tradeHistoryProxy;
                tradeHistoryProxy.setSourceModel(tradeHistoryAbstractModel);
                tradeHistoryProxy.setFilterKeyColumn(0);
                tradeHistoryProxy.setFilterFixedString(QString::fromStdString(hash.GetHex()));
                QModelIndex rowIndex = tradeHistoryProxy.mapToSource(tradeHistoryProxy.index(0,0)); // map to the row in the actual table
                if(rowIndex.isValid()) ui->tradeHistoryTable->removeRow(rowIndex.row()); // delete the pending tx row, it'll be readded as a proper confirmed transaction
                ui->tradeHistoryTable->setSortingEnabled(true); // re-enable sorting
            }
        }

        // tx not in historyMap, retrieve the transaction object
        CTransaction wtx;
        uint256 blockHash;
        if (!GetTransaction(hash, wtx, Params().GetConsensus(), blockHash, true)) continue;
        if (blockHash.IsNull() || NULL == GetBlockIndex(blockHash)) continue;
        CBlockIndex* pBlockIndex = GetBlockIndex(blockHash);
        if (NULL == pBlockIndex) continue;
        int blockHeight = pBlockIndex->nHeight;

        // setup some variables
        CMPTransaction mp_obj;
        std::string statusText;
        uint32_t propertyIdForSale = 0;
        uint32_t propertyIdDesired = 0;
        int64_t amountForSale = 0;
        int64_t amountDesired = 0;
        bool divisibleForSale = false;
        bool divisibleDesired = false;
        UniValue tradeArray(UniValue::VARR);
        int64_t totalReceived = 0;
        int64_t totalSold = 0;
        bool orderOpen = false;
        bool valid = false;

        // parse the transaction
        if (0 != ParseTransaction(wtx, blockHeight, 0, mp_obj)) continue;
        if (mp_obj.interpret_Transaction()) {
            valid = getValidMPTX(hash);
            propertyIdForSale = mp_obj.getProperty();
            amountForSale = mp_obj.getAmount();
            divisibleForSale = isPropertyDivisible(propertyIdForSale);
            CMPMetaDEx temp_metadexoffer(mp_obj);
            propertyIdDesired = temp_metadexoffer.getDesProperty();
            divisibleDesired = isPropertyDivisible(propertyIdDesired);
            amountDesired = temp_metadexoffer.getAmountDesired();
            {
                LOCK(cs_tally);
                t_tradelistdb->getMatchingTrades(hash, propertyIdForSale, tradeArray, totalSold, totalReceived);
                orderOpen = MetaDEx_isOpen(hash, propertyIdForSale);
            }
        }

        // work out status
        bool partialFilled = false;
        bool filled = false;
        statusText = "Unknown";
        if (totalSold > 0) partialFilled = true;
        if (totalSold >= amountForSale) filled = true;
        if (!orderOpen && !partialFilled) statusText = "Cancelled";
        if (!orderOpen && partialFilled) statusText = "Part Cancel";
        if (!orderOpen && filled) statusText = "Filled";
        if (orderOpen && !partialFilled) statusText = "Open";
        if (orderOpen && partialFilled) statusText = "Part Filled";
        if (!valid) statusText = "Invalid";

        // prepare display values
        std::string displayText = "Sell ";
        if (divisibleForSale) { displayText += FormatDivisibleShortMP(amountForSale); } else { displayText += FormatIndivisibleMP(amountForSale); }
        displayText += getTokenLabel(propertyIdForSale) + " for ";
        if (divisibleDesired) { displayText += FormatDivisibleShortMP(amountDesired); } else { displayText += FormatIndivisibleMP(amountDesired); }
        displayText += getTokenLabel(propertyIdDesired);
        std::string displayIn = "";
        std::string displayOut = "-";
        if(divisibleDesired) { displayIn += FormatDivisibleShortMP(totalReceived); } else { displayIn += FormatIndivisibleMP(totalReceived); }
        if(divisibleForSale) { displayOut += FormatDivisibleShortMP(totalSold); } else { displayOut += FormatIndivisibleMP(totalSold); }
        if(totalReceived == 0) displayIn = "0";
        if(totalSold == 0) displayOut = "0";
        displayIn += getTokenLabel(propertyIdDesired);
        displayOut += getTokenLabel(propertyIdForSale);

        // create a TradeHistoryObject and populate it
        TradeHistoryObject objTH;
        objTH.blockHeight = blockHeight;
        objTH.valid = valid;
        objTH.propertyIdForSale = propertyIdForSale;
        objTH.propertyIdDesired = propertyIdDesired;
        objTH.amountForSale = amountForSale;
        objTH.status = statusText;
        objTH.amountIn = displayIn;
        objTH.amountOut = displayOut;
        objTH.info = displayText;

        // add the new TradeHistoryObject to the map
        tradeHistoryMap.insert(std::make_pair(hash, objTH));
        nProcessed += 1;
    }
    // ### END WALLET TRANSACTIONS PROCESSING ###

    return nProcessed;
}

// Each time an update is called it's feasible that the status and amounts traded for open trades may have changed
// This function will loop through each of the rows in tradeHistoryTable and check if the details need updating
void TradeHistoryDialog::UpdateData()
{
    int chainHeight = GetHeight();
    int rowCount = ui->tradeHistoryTable->rowCount();
    ui->tradeHistoryTable->setSortingEnabled(false); // disable sorting while we update the table
    for (int row = 0; row < rowCount; row++) {
        // check if we need to refresh the details for this row
        int lastUpdateBlock = ui->tradeHistoryTable->item(row,1)->text().toInt();
        uint256 txid;
        txid.SetHex(ui->tradeHistoryTable->item(row,0)->text().toStdString());
        TradeHistoryMap::iterator hIter = tradeHistoryMap.find(txid);
        if (hIter == tradeHistoryMap.end()) {
            PrintToLog("UI Error: Transaction %s appears in tradeHistoryTable but cannot be found in tradeHistoryMap.\n", txid.GetHex());
            continue;
        }
        TradeHistoryObject *tmpObjTH = &(hIter->second);
        if (tmpObjTH->status == "Filled" || tmpObjTH->status == "Cancelled" || tmpObjTH->status == "Part Cancel" || tmpObjTH->status == "Invalid") continue; // once a trade hits this status the details should never change
        if (tmpObjTH->blockHeight == 0) continue; // do not attempt to refresh details for a trade that's still pending
        if (lastUpdateBlock == chainHeight) continue; // no new blocks since last update, don't waste compute looking for updates

        // at this point we have an active trade and there have been new block(s) since the last update - refresh status and amounts
        uint32_t propertyIdForSale = tmpObjTH->propertyIdForSale;
        uint32_t propertyIdDesired = tmpObjTH->propertyIdDesired;
        int64_t amountForSale = tmpObjTH->amountForSale;
        UniValue tradeArray(UniValue::VARR);
        int64_t totalReceived = 0;
        int64_t totalSold = 0;
        bool orderOpen = false;
        {
            LOCK(cs_tally);
            t_tradelistdb->getMatchingTrades(txid, propertyIdForSale, tradeArray, totalSold, totalReceived);
            orderOpen = MetaDEx_isOpen(txid, propertyIdForSale);
        }
        // work out new status & icon
        bool partialFilled = false;
        bool filled = false;
        if (totalSold > 0) partialFilled = true;
        if (totalSold >= amountForSale) filled = true;
        std::string statusText = "Unknown";
        QIcon ic = QIcon(":/icons/transaction_0");
        if (!orderOpen && !partialFilled) { statusText = "Cancelled"; ic = QIcon(":/icons/meta_cancelled"); }
        if (!orderOpen && partialFilled) { statusText = "Part Cancel"; ic = QIcon(":/icons/meta_partialclosed"); }
        if (!orderOpen && filled) { statusText = "Filled"; ic = QIcon(":/icons/meta_filled"); }
        if (orderOpen && !partialFilled) { statusText = "Open"; ic = QIcon(":/icons/meta_open"); }
        if (orderOpen && partialFilled) { statusText = "Part Filled"; ic = QIcon(":/icons/meta_partial"); }
        if (!tmpObjTH->valid) { statusText = "Invalid"; ic = QIcon(":/icons/transaction_invalid"); }

        // format new amounts
        std::string displayIn = "";
        std::string displayOut = "-";
        if (isPropertyDivisible(propertyIdDesired)) { displayIn += FormatDivisibleShortMP(totalReceived); } else { displayIn += FormatIndivisibleMP(totalReceived); }
        if (isPropertyDivisible(propertyIdForSale)) { displayOut += FormatDivisibleShortMP(totalSold); } else { displayOut += FormatIndivisibleMP(totalSold); }
        if (totalReceived == 0) displayIn = "0";
        if (totalSold == 0) displayOut = "0";
        displayIn += getTokenLabel(propertyIdDesired);
        displayOut += getTokenLabel(propertyIdForSale);

        // create and format replacement cells
        QTableWidgetItem *lastUpdateBlockCell = new QTableWidgetItem(QString::fromStdString(FormatIndivisibleMP(chainHeight)));
        QTableWidgetItem *statusCell = new QTableWidgetItem(QString::fromStdString(statusText));
        QTableWidgetItem *amountOutCell = new QTableWidgetItem(QString::fromStdString(displayOut));
        QTableWidgetItem *amountInCell = new QTableWidgetItem(QString::fromStdString(displayIn));
        QTableWidgetItem *iconCell = new QTableWidgetItem;
        QTableWidgetItem *dateCell = new QTableWidgetItem;
        dateCell->setData(Qt::DisplayRole, ui->tradeHistoryTable->item(row, 3)->data(Qt::DisplayRole)); // values don't change so can be copied
        QTableWidgetItem *infoCell = new QTableWidgetItem(ui->tradeHistoryTable->item(row, 5)->text()); // as above
        iconCell->setIcon(ic);
        amountOutCell->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
        amountOutCell->setForeground(QColor("#EE0000"));
        amountInCell->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
        amountInCell->setForeground(QColor("#00AA00"));
        if (statusText == "Cancelled" || statusText == "Filled" || statusText == "Part Cancel" || statusText == "Invalid") {
            // dull the colors for non-active trades
            dateCell->setForeground(QColor("#707070"));
            statusCell->setForeground(QColor("#707070"));
            infoCell->setForeground(QColor("#707070"));
            amountOutCell->setForeground(QColor("#993333"));
            amountInCell->setForeground(QColor("#006600"));
        }
        if(displayIn.substr(0,2) == "0 ") amountInCell->setForeground(QColor("#000000"));
        if(displayOut.substr(0,2) == "0 ") amountOutCell->setForeground(QColor("#000000"));
        // replace cells in row accordingly
        ui->tradeHistoryTable->setItem(row, 1, lastUpdateBlockCell);
        ui->tradeHistoryTable->setItem(row, 2, iconCell);
        ui->tradeHistoryTable->setItem(row, 3, dateCell);
        ui->tradeHistoryTable->setItem(row, 4, statusCell);
        ui->tradeHistoryTable->setItem(row, 5, infoCell);
        ui->tradeHistoryTable->setItem(row, 6, amountOutCell);
        ui->tradeHistoryTable->setItem(row, 7, amountInCell);
    }
    ui->tradeHistoryTable->setSortingEnabled(true); // re-enable sorting
}

void TradeHistoryDialog::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if (model != NULL) { } // do nothing, signals from walletModel no longer needed
}

void TradeHistoryDialog::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if (model != NULL) {
        connect(model, SIGNAL(refreshOmniBalance()), this, SLOT(UpdateTradeHistoryTable()));
        connect(model, SIGNAL(reinitOmniState()), this, SLOT(ReinitTradeHistoryTable()));
    }
}

void TradeHistoryDialog::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tradeHistoryTable->indexAt(point);
    if(index.isValid()) contextMenu->exec(QCursor::pos());
}

void TradeHistoryDialog::copyTxID()
{
    GUIUtil::setClipboard(ui->tradeHistoryTable->item(ui->tradeHistoryTable->currentRow(),0)->text());
}

/* Opens a dialog containing the details of the selected trade and any associated matches
 */
void TradeHistoryDialog::showDetails()
{
    UniValue txobj(UniValue::VOBJ);;
    uint256 txid;
    txid.SetHex(ui->tradeHistoryTable->item(ui->tradeHistoryTable->currentRow(),0)->text().toStdString());
    std::string strTXText;

    if (!txid.IsNull()) {
        // grab extended trade details via the RPC populator
        int rc = populateRPCTransactionObject(txid, txobj, "", true);
        if (rc >= 0) strTXText = txobj.write(true);
    }

    if (!strTXText.empty()) {
        PopulateSimpleDialog(strTXText, "Trade Information", "Trade Information");
    }
}

void TradeHistoryDialog::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    borrowedColumnResizingFixer->stretchColumnWidth(5);
}

