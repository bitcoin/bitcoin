// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txhistorydialog.h"
#include "ui_txhistorydialog.h"

#include "omnicore_qtutils.h"

#include "clientmodel.h"
#include "guiutil.h"
#include "walletmodel.h"

#include "mastercore.h"
#include "mastercore_rpc.h"
#include "mastercore_sp.h"
#include "mastercore_tx.h"
#include "omnicore_pending.h"

#include "init.h"
#include "main.h"
#include "primitives/transaction.h"
#include "sync.h"
#include "txdb.h"
#include "uint256.h"
#include "wallet.h"

#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <stdint.h>
#include <list>
#include <map>
#include <string>
#include <vector>

#include <QAbstractItemModel>
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
#include <QTableWidgetItem>
#include <QWidget>

using std::string;
using namespace json_spirit;
using namespace mastercore;

TXHistoryDialog::TXHistoryDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::txHistoryDialog),
    clientModel(0),
    walletModel(0)
{
    ui->setupUi(this);
    // setup
    ui->txHistoryTable->setColumnCount(7);
    ui->txHistoryTable->setHorizontalHeaderItem(2, new QTableWidgetItem(" "));
    ui->txHistoryTable->setHorizontalHeaderItem(3, new QTableWidgetItem("Date"));
    ui->txHistoryTable->setHorizontalHeaderItem(4, new QTableWidgetItem("Type"));
    ui->txHistoryTable->setHorizontalHeaderItem(5, new QTableWidgetItem("Address"));
    ui->txHistoryTable->setHorizontalHeaderItem(6, new QTableWidgetItem("Amount"));
    // borrow ColumnResizingFixer again
    borrowedColumnResizingFixer = new GUIUtil::TableViewLastColumnResizingFixer(ui->txHistoryTable,100,100);
    // allow user to adjust - go interactive then manually set widths
    #if QT_VERSION < 0x050000
       ui->txHistoryTable->horizontalHeader()->setResizeMode(2, QHeaderView::Fixed);
       ui->txHistoryTable->horizontalHeader()->setResizeMode(3, QHeaderView::Interactive);
       ui->txHistoryTable->horizontalHeader()->setResizeMode(4, QHeaderView::Interactive);
       ui->txHistoryTable->horizontalHeader()->setResizeMode(5, QHeaderView::Interactive);
       ui->txHistoryTable->horizontalHeader()->setResizeMode(6, QHeaderView::Interactive);
   #else
       ui->txHistoryTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
       ui->txHistoryTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive);
       ui->txHistoryTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Interactive);
       ui->txHistoryTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Interactive);
       ui->txHistoryTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Interactive);
    #endif
    ui->txHistoryTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->txHistoryTable->verticalHeader()->setVisible(false);
    ui->txHistoryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->txHistoryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->txHistoryTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->txHistoryTable->setTabKeyNavigation(false);
    ui->txHistoryTable->setContextMenuPolicy(Qt::CustomContextMenu);
    // set alternating row colors via styling instead of manually
    ui->txHistoryTable->setAlternatingRowColors(true);
    // Actions
    QAction *copyAddressAction = new QAction(tr("Copy address"), this);
    QAction *copyAmountAction = new QAction(tr("Copy amount"), this);
    QAction *copyTxIDAction = new QAction(tr("Copy transaction ID"), this);
    QAction *showDetailsAction = new QAction(tr("Show transaction details"), this);
    contextMenu = new QMenu();
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyAmountAction);
    contextMenu->addAction(copyTxIDAction);
    contextMenu->addAction(showDetailsAction);
    // Connect actions
    connect(ui->txHistoryTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));
    connect(ui->txHistoryTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(showDetails()));
    connect(ui->txHistoryTable->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(checkSort(int)));
    connect(copyAddressAction, SIGNAL(triggered()), this, SLOT(copyAddress()));
    connect(copyAmountAction, SIGNAL(triggered()), this, SLOT(copyAmount()));
    connect(copyTxIDAction, SIGNAL(triggered()), this, SLOT(copyTxID()));
    connect(showDetailsAction, SIGNAL(triggered()), this, SLOT(showDetails()));
    // Perform initial population and update of history table

    UpdateHistory();
    // since there is no model we can't do this before we add some data, so now let's do the resizing
    // *after* we've populated the initial content for the table
    ui->txHistoryTable->setColumnWidth(2, 23);
    ui->txHistoryTable->resizeColumnToContents(3);
    ui->txHistoryTable->resizeColumnToContents(4);
    ui->txHistoryTable->resizeColumnToContents(6);
    ui->txHistoryTable->setColumnHidden(0, true); // hidden txid for displaying transaction details
    ui->txHistoryTable->setColumnHidden(1, true); // hideen sort key
    borrowedColumnResizingFixer->stretchColumnWidth(5);
    ui->txHistoryTable->setSortingEnabled(true);
    ui->txHistoryTable->horizontalHeader()->setSortIndicator(1, Qt::DescendingOrder); // sort by hidden sort key

}

TXHistoryDialog::~TXHistoryDialog()
{
    delete ui;
}

void TXHistoryDialog::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if (model != NULL) {
        connect(model, SIGNAL(refreshOmniState()), this, SLOT(UpdateHistory()));
        connect(model, SIGNAL(numBlocksChanged(int)), this, SLOT(UpdateConfirmations()));
    }
}

void TXHistoryDialog::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if (model != NULL) {
        connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(UpdateHistory()));
    }
}

int TXHistoryDialog::PopulateHistoryMap()
{
    CWallet *wallet = pwalletMain;
    if (NULL == wallet) return 0;
    // try and fix intermittent freeze on startup and while running by only updating if we can get required locks
    TRY_LOCK(cs_main,lckMain);
    if (!lckMain) return 0;
    TRY_LOCK(wallet->cs_wallet, lckWallet);
    if (!lckWallet) return 0;

    int64_t nProcessed = 0; // counter for how many transactions we've added to history this time
    // ### START PENDING TRANSACTIONS PROCESSING ###
    for(PendingMap::iterator it = my_pending.begin(); it != my_pending.end(); ++it)
    {
        // grab txid
        uint256 txid = it->first;
        // check historyMap, if this tx exists don't waste resources doing anymore work on it
        HistoryMap::iterator hIter = txHistoryMap.find(txid);
        if (hIter != txHistoryMap.end()) continue;
        // grab pending object & extract details
        CMPPending *p_pending = &(it->second);
        string senderAddress = p_pending->src;
        uint64_t propertyId = p_pending->prop;
        int64_t amount = p_pending->amount;
        // create a HistoryTXObject and add to map
        HistoryTXObject htxo;
        htxo.blockHeight = 0; // how are we gonna order pending txs????
        htxo.blockByteOffset = 0; // attempt to use the position of the transaction in the wallet to provide a sortkey for pending
        std::map<uint256, CWalletTx>::const_iterator walletIt = wallet->mapWallet.find(txid);
        if (walletIt != wallet->mapWallet.end()) {
            const CWalletTx* pendingWTx = &(*walletIt).second;
            htxo.blockByteOffset = pendingWTx->nOrderPos;
        }
        htxo.valid = true; // all pending transactions are assumed to be valid while awaiting confirmation since all pending are outbound and we wouldn't let them be sent if invalid
        if (p_pending->type == 0) { htxo.txType = "Send"; htxo.fundsMoved = true; } // we don't have a CMPTransaction class here so manually set the type for now
        if (p_pending->type == 21) { htxo.txType = "MetaDEx Trade"; htxo.fundsMoved = false; } // send and metadex trades are the only supported outbound txs (thus only possible pending) for now
        htxo.address = senderAddress; // always sender, all pending are outbound
        if(isPropertyDivisible(propertyId)) {htxo.amount = "-"+FormatDivisibleShortMP(amount)+getTokenLabel(propertyId);} else {htxo.amount="-"+FormatIndivisibleMP(amount)+getTokenLabel(propertyId);} // pending always outbound
        txHistoryMap.insert(std::make_pair(txid, htxo));
        nProcessed += 1; // increase our counter so we don't go crazy on a wallet with too many transactions and lock up UI
    }
    // ### END PENDING TRANSACTIONS PROCESSING ###

    // ### START WALLET TRANSACTIONS PROCESSING ###
    // STO has no inbound transaction, so we need to use an insert methodology here - get STO receipts affecting me
    string mySTOReceipts = s_stolistdb->getMySTOReceipts("");
    std::vector<std::string> vecReceipts;
    boost::split(vecReceipts, mySTOReceipts, boost::is_any_of(","), boost::token_compress_on);
    int64_t lastTXBlock = 999999; // set artificially high initially until first wallet tx is processed
    // iterate through wallet entries backwards
    std::list<CAccountingEntry> acentries;
    CWallet::TxItems txOrdered = pwalletMain->OrderedTxItems(acentries, "*");
    for (CWallet::TxItems::reverse_iterator it = txOrdered.rbegin(); it != txOrdered.rend(); ++it) {
        CWalletTx *const pwtx = (*it).second.first;
        if (pwtx != 0) {
            uint256 hash = pwtx->GetHash();
            // check txlistdb, if not there it's not a confirmed Omni transaction so move to next transaction
            if (!p_txlistdb->exists(hash)) continue;
            // check historyMap, if this tx exists don't waste resources doing anymore work on it
            HistoryMap::iterator hIter = txHistoryMap.find(hash);
            if (hIter != txHistoryMap.end()) { // the tx is in historyMap, check if it has a blockheight of 0 (means a pending has confirmed)
                HistoryTXObject *temphtxo = &(hIter->second);
                if (temphtxo->blockHeight == 0) { // pending has confirmed, delete the pending transaction from the map/table and have it parsed/readded properly
                    txHistoryMap.erase(hIter);
                    ui->txHistoryTable->setSortingEnabled(false); // disable sorting temporarily while we update the table (leaving enabled gives unexpected results)
                    QAbstractItemModel* historyAbstractModel = ui->txHistoryTable->model(); // get a model to work with
                    QSortFilterProxyModel historyProxy;
                    historyProxy.setSourceModel(historyAbstractModel);
                    historyProxy.setFilterKeyColumn(0);
                    historyProxy.setFilterFixedString(QString::fromStdString(hash.GetHex()));
                    QModelIndex rowIndex = historyProxy.mapToSource(historyProxy.index(0,0)); // map to the row in the actual table
                    if(rowIndex.isValid()) ui->txHistoryTable->removeRow(rowIndex.row()); // delete the pending tx row, it'll be readded as a proper confirmed transaction
                    ui->txHistoryTable->setSortingEnabled(true); // re-enable sorting
                } else {
                    continue; // the tx is in historyMap with a blockheight > 0, move on to next transaction
                }
            }
            // tx not in historyMap, retrieve the transaction object
            CTransaction wtx;
            uint256 blockHash = 0;
            if (!GetTransaction(hash, wtx, blockHash, true)) continue;
            blockHash = pwtx->hashBlock;
            if ((0 == blockHash) || (NULL == GetBlockIndex(blockHash))) continue;
            CBlockIndex* pBlockIndex = GetBlockIndex(blockHash);
            if (NULL == pBlockIndex) continue;
            int blockHeight = pBlockIndex->nHeight; // get the height of the transaction

            // ### START STO INSERTION CHECK ###
            for(uint32_t i = 0; i<vecReceipts.size(); i++) {
                std::vector<std::string> svstr;
                boost::split(svstr, vecReceipts[i], boost::is_any_of(":"), boost::token_compress_on);
                if(4 == svstr.size()) // make sure expected num items
                {
                    if((atoi(svstr[1]) < lastTXBlock) && (atoi(svstr[1]) > blockHeight))
                    {
                        // STO receipt insert here - add STO receipt to response array
                        uint256 stoHash;
                        stoHash.SetHex(svstr[0]);
                        // check historyMap, if this STO exists don't waste resources doing anymore work on it
                        HistoryMap::iterator hIterator = txHistoryMap.find(stoHash);
                        if (hIterator != txHistoryMap.end()) continue;
                        // STO not in historyMap, get details
                        uint64_t propertyId = 0;
                        try { propertyId = boost::lexical_cast<uint64_t>(svstr[3]); } // attempt to extract propertyId
                          catch (const boost::bad_lexical_cast &e) { file_log("DEBUG STO - error in converting values from leveldb\n"); continue; } //(something went wrong)
                        Array receiveArray;
                        uint64_t total = 0;
                        uint64_t stoFee = 0;
                        s_stolistdb->getRecipients(stoHash, "", &receiveArray, &total, &stoFee); // get matching receipts
                        // create a HistoryTXObject and add to map
                        HistoryTXObject htxo;
                        htxo.blockHeight = atoi(svstr[1]);
                        htxo.blockByteOffset = 0;
                        CDiskTxPos position;
                        if (pblocktree->ReadTxIndex(stoHash, position)) {
                            htxo.blockByteOffset = position.nTxOffset;
                        }
                        htxo.txType = "STO Receive";
                        htxo.valid = true; // STO receipts are always valid (STO sends not necessarily so, but this section only handles receipts)
                        htxo.address = svstr[2];
                        htxo.fundsMoved = true; // receiving tokens means tokens moved
                        if(receiveArray.size()>1) htxo.address = "Multiple addresses"; // override display address if more than one address in the wallet received a cut of this STO
                        if(isPropertyDivisible(propertyId)) {htxo.amount=FormatDivisibleShortMP(total)+getTokenLabel(propertyId);} else {htxo.amount=FormatIndivisibleMP(total)+getTokenLabel(propertyId);}
                        txHistoryMap.insert(std::make_pair(stoHash, htxo));
                        nProcessed += 1; // increase our counter so we don't go crazy on a wallet with too many transactions and lock up UI
                    }
                }
            }
            lastTXBlock = blockHeight;
            // ### END STO INSERTION CHECK ###

            // continuing with wallet tx, we've already confirmed earlier on that it's in txlistdb
            string statusText;
            unsigned int propertyId = 0;
            uint64_t amount = 0;
            string senderAddress;
            string refAddress;
            bool divisible = false;
            bool valid = false;
            string MPTxType;
            CMPTransaction mp_obj;
            int parseRC = parseTransaction(true, wtx, blockHeight, 0, &mp_obj);
            string displayAmount;
            string displayToken;
            string displayValid;
            string displayAddress;
            string displayType;
            if (0 < parseRC) { //positive RC means payment
                // ### START HANDLE DEX PURCHASE TRANSACTION ###
                string tmpBuyer;
                string tmpSeller;
                uint64_t total = 0;
                uint64_t tmpVout = 0;
                uint64_t tmpNValue = 0;
                uint64_t tmpPropertyId = 0;
                p_txlistdb->getPurchaseDetails(hash,1,&tmpBuyer,&tmpSeller,&tmpVout,&tmpPropertyId,&tmpNValue);
                bool bIsBuy = IsMyAddress(tmpBuyer);
                int numberOfPurchases=p_txlistdb->getNumberOfPurchases(hash);
                if (0<numberOfPurchases) // calculate total bought/sold
                {
                    for(int purchaseNumber = 1; purchaseNumber <= numberOfPurchases; purchaseNumber++)
                    {
                        p_txlistdb->getPurchaseDetails(hash,purchaseNumber,&tmpBuyer,&tmpSeller,&tmpVout,&tmpPropertyId,&tmpNValue);
                        total += tmpNValue;
                    }
                    // create a HistoryTXObject and add to map
                    HistoryTXObject htxo;
                    htxo.blockHeight = blockHeight;
                    htxo.blockByteOffset = 0; // needs further investigation
                    if (!bIsBuy) { htxo.txType = "DEx Sell"; htxo.address = tmpSeller; } else { htxo.txType = "DEx Buy"; htxo.address = tmpBuyer; }
                    htxo.amount=(!bIsBuy ? "-" : "") + FormatDivisibleShortMP(total)+getTokenLabel(tmpPropertyId);
                    htxo.fundsMoved=true;
                    txHistoryMap.insert(std::make_pair(hash, htxo));
                    nProcessed += 1; // increase our counter so we don't go crazy on a wallet with too many transactions and lock up UI
                }
                // ### END HANDLE DEX PURCHASE TRANSACTION ###
            }
            if (0 == parseRC) { //negative RC means no MP content/badly encoded TX, we shouldn't see this if TX in levelDB but check for sanity
                // ### START HANDLE OMNI TRANSACTION ###
                if (0<=mp_obj.step1()) {
                    MPTxType = mp_obj.getTypeString();
                    senderAddress = mp_obj.getSender();
                    refAddress = mp_obj.getReceiver();
                    int tmpblock=0;
                    uint32_t tmptype=0;
                    uint64_t amountNew=0;
                    valid=getValidMPTX(hash, &tmpblock, &tmptype, &amountNew);
                    if (0 == mp_obj.step2_Value()) {
                        propertyId = mp_obj.getProperty();
                        amount = mp_obj.getAmount();
                        // special case for property creation (getProperty cannot get ID as createdID not stored in obj)
                        if (valid) { // we only generate an ID for valid creates
                            if ((mp_obj.getType() == MSC_TYPE_CREATE_PROPERTY_FIXED) ||
                                (mp_obj.getType() == MSC_TYPE_CREATE_PROPERTY_VARIABLE) ||
                                (mp_obj.getType() == MSC_TYPE_CREATE_PROPERTY_MANUAL)) {
                                    propertyId = _my_sps->findSPByTX(hash);
                                    if (mp_obj.getType() == MSC_TYPE_CREATE_PROPERTY_FIXED) { amount = getTotalTokens(propertyId); } else { amount = 0; }
                                }
                        }
                        divisible = isPropertyDivisible(propertyId);
                    }
                }
                bool fundsMoved = true;
                displayType = shrinkTxType(mp_obj.getType(), &fundsMoved); // shrink tx type to better fit in column
                if (IsMyAddress(senderAddress)) { displayAddress = senderAddress; } else { displayAddress = refAddress; }
                if (divisible) { displayAmount = FormatDivisibleShortMP(amount)+getTokenLabel(propertyId); } else { displayAmount = FormatIndivisibleMP(amount)+getTokenLabel(propertyId); }
                if ((displayType == "Send") && (!IsMyAddress(senderAddress))) { displayType = "Receive"; } // still a send transaction, but avoid confusion for end users
                if (!valid) fundsMoved = false; // funds never move in invalid txs
                // override/hide display amount for invalid creates and unknown transactions as we can't display amount/property as no prop exists
                if ((mp_obj.getType() == MSC_TYPE_CREATE_PROPERTY_FIXED) ||
                    (mp_obj.getType() == MSC_TYPE_CREATE_PROPERTY_VARIABLE) ||
                    (mp_obj.getType() == MSC_TYPE_CREATE_PROPERTY_MANUAL) ||
                    (displayType == "Unknown")) {
                        // alerts are valid and thus display a wacky value attempting to decode amount - whilst no users should ever see this, be clean and N/A the wacky value
                        if ((!valid) || (displayType == "Unknown")) { displayAmount = "N/A"; }
                } else { if ((fundsMoved) && (IsMyAddress(senderAddress))) { displayAmount = "-" + displayAmount; } }
                // create a HistoryTXObject and add to map
                HistoryTXObject htxo;
                htxo.blockHeight = blockHeight;
                htxo.blockByteOffset = 0;
                CDiskTxPos position;
                if (pblocktree->ReadTxIndex(hash, position)) {
                    htxo.blockByteOffset = position.nTxOffset;
                }
                htxo.txType = displayType;
                htxo.address = displayAddress;
                htxo.valid = valid; // bool valid contains result from getValidMPTX
                htxo.fundsMoved = fundsMoved;
                htxo.amount = displayAmount;
                txHistoryMap.insert(std::make_pair(hash, htxo));
                nProcessed += 1; // increase our counter so we don't go crazy on a wallet with too many transactions and lock up UI
                // ### END HANDLE OMNI TRANSACTION ###
            }
        }
        // display cap has been removed
    }

    // ### END WALLET TRANSACTIONS PROCESSING ###
    return nProcessed;
}

void TXHistoryDialog::UpdateConfirmations()
{
    int chainHeight = chainActive.Height(); // get the chain height
    int rowCount = ui->txHistoryTable->rowCount();
    for (int row = 0; row < rowCount; row++) {
        int confirmations = 0;
        bool valid = false;
        uint256 txid;
        txid.SetHex(ui->txHistoryTable->item(row,0)->text().toStdString());
        // lookup transaction in the map and grab validity and blockheight details
        HistoryMap::iterator hIter = txHistoryMap.find(txid);
        if (hIter != txHistoryMap.end()) {
            HistoryTXObject *temphtxo = &(hIter->second);
            if (temphtxo->blockHeight>0) confirmations = (chainHeight+1) - temphtxo->blockHeight;
            valid = temphtxo->valid;
        }
        int txBlockHeight = ui->txHistoryTable->item(row,1)->text().toInt();
        if (txBlockHeight>0) confirmations = (chainHeight+1) - txBlockHeight;
        // setup the appropriate icon
        QIcon ic = QIcon(":/icons/transaction_0");
        switch(confirmations) {
            case 1: ic = QIcon(":/icons/transaction_1"); break;
            case 2: ic = QIcon(":/icons/transaction_2"); break;
            case 3: ic = QIcon(":/icons/transaction_3"); break;
            case 4: ic = QIcon(":/icons/transaction_4"); break;
            case 5: ic = QIcon(":/icons/transaction_5"); break;
        }
        if (confirmations > 5) ic = QIcon(":/icons/transaction_confirmed");
        if (!valid) ic = QIcon(":/icons/transaction_invalid");
        QTableWidgetItem *iconCell = new QTableWidgetItem;
        iconCell->setIcon(ic);
        ui->txHistoryTable->setItem(row, 2, iconCell);
    }
}

void TXHistoryDialog::UpdateHistory()
{
    // now moved to a new methodology where historical transactions are stored in a map in memory (effectively a cache) so we can compare our
    // history table against the cache.  This allows us to avoid reparsing transactions repeatedly and provides a diff to modify the table without
    // repopuplating all the rows top to bottom each refresh.

    // first things first, call PopulateHistoryMap to add in any missing (ie new) transactions
    int newTXCount = PopulateHistoryMap();
    // were any transactions added?
    if(newTXCount > 0) { // there are new transactions (or a pending shifted to confirmed), refresh the table adding any that are in the map but not in the table
        ui->txHistoryTable->setSortingEnabled(false); // disable sorting temporarily while we update the table (leaving enabled gives unexpected results)
        QAbstractItemModel* historyAbstractModel = ui->txHistoryTable->model(); // get a model to work with
        for(HistoryMap::iterator it = txHistoryMap.begin(); it != txHistoryMap.end(); ++it) {
            uint256 txid = it->first; // grab txid
            // search the history table for this transaction, here we're going to use a filter proxy because it's a little quicker
            QSortFilterProxyModel historyProxy;
            historyProxy.setSourceModel(historyAbstractModel);
            historyProxy.setFilterKeyColumn(0);
            historyProxy.setFilterFixedString(QString::fromStdString(txid.GetHex()));
            QModelIndex rowIndex = historyProxy.mapToSource(historyProxy.index(0,0));
            if(!rowIndex.isValid()) { // this transaction doesn't exist in the history table, add it
                HistoryTXObject htxo = it->second; // grab the tranaaction
                int workingRow = ui->txHistoryTable->rowCount();
                ui->txHistoryTable->insertRow(workingRow); // append a new row (sorting will take care of ordering)
                QDateTime txTime;
                QTableWidgetItem *dateCell = new QTableWidgetItem;
                if (htxo.blockHeight>0) {
                    CBlockIndex* pBlkIdx = chainActive[htxo.blockHeight];
                    if (NULL != pBlkIdx) txTime.setTime_t(pBlkIdx->GetBlockTime());
                    dateCell->setData(Qt::DisplayRole, txTime);
                } else {
                    dateCell->setData(Qt::DisplayRole, QString::fromStdString("Unconfirmed"));
                }
                QTableWidgetItem *typeCell = new QTableWidgetItem(QString::fromStdString(htxo.txType));
                QTableWidgetItem *addressCell = new QTableWidgetItem(QString::fromStdString(htxo.address));
                QTableWidgetItem *amountCell = new QTableWidgetItem(QString::fromStdString(htxo.amount));
                QTableWidgetItem *iconCell = new QTableWidgetItem;
                QTableWidgetItem *txidCell = new QTableWidgetItem(QString::fromStdString(txid.GetHex()));
                std::string sortKey = strprintf("%06d%010d",htxo.blockHeight,htxo.blockByteOffset);
                if(htxo.blockHeight == 0) sortKey = strprintf("%06d%010D",999999,htxo.blockByteOffset); // spoof the hidden value to ensure pending txs are sorted top
                QTableWidgetItem *sortKeyCell = new QTableWidgetItem(QString::fromStdString(sortKey));
                addressCell->setTextAlignment(Qt::AlignLeft + Qt::AlignVCenter);
                addressCell->setForeground(QColor("#707070"));
                amountCell->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
                amountCell->setForeground(QColor("#00AA00"));
                if (htxo.amount.length() > 0) { // protect against an empty value
                    if (htxo.amount.substr(0,1) == "-") amountCell->setForeground(QColor("#EE0000")); // outbound
                }
                if (!htxo.fundsMoved) amountCell->setForeground(QColor("#404040"));
                ui->txHistoryTable->setItem(workingRow, 0, txidCell);
                ui->txHistoryTable->setItem(workingRow, 1, sortKeyCell);
                ui->txHistoryTable->setItem(workingRow, 2, iconCell);
                ui->txHistoryTable->setItem(workingRow, 3, dateCell);
                ui->txHistoryTable->setItem(workingRow, 4, typeCell);
                ui->txHistoryTable->setItem(workingRow, 5, addressCell);
                ui->txHistoryTable->setItem(workingRow, 6, amountCell);
            }
        }
        ui->txHistoryTable->setSortingEnabled(true); // re-enable sorting
    }
    UpdateConfirmations();
}

void TXHistoryDialog::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->txHistoryTable->indexAt(point);
    if(index.isValid())
    {
        contextMenu->exec(QCursor::pos());
    }
}

void TXHistoryDialog::copyAddress()
{
    GUIUtil::setClipboard(ui->txHistoryTable->item(ui->txHistoryTable->currentRow(),5)->text());
}

void TXHistoryDialog::copyAmount()
{
    GUIUtil::setClipboard(ui->txHistoryTable->item(ui->txHistoryTable->currentRow(),6)->text());
}

void TXHistoryDialog::copyTxID()
{
    GUIUtil::setClipboard(ui->txHistoryTable->item(ui->txHistoryTable->currentRow(),0)->text());
}

void TXHistoryDialog::checkSort(int column)
{
    // a header has been clicked on the tx history table, see if it's the status column and override the sort if necessary
    // we may wish to be a bit smarter about this longer term, so we can spoof a sort indicator display and perhaps allow both
    // directions, but for now will provide the core functionality needed
    if (column == 2) { // ignore any other column sorts and allow them to progress normally
        ui->txHistoryTable->horizontalHeader()->setSortIndicator(1, Qt::DescendingOrder);
    }
}

void TXHistoryDialog::showDetails()
{
    Object txobj;
    uint256 txid;
    txid.SetHex(ui->txHistoryTable->item(ui->txHistoryTable->currentRow(),0)->text().toStdString());
    std::string strTXText;

    // first of all check if the TX is a pending tx, if so grab details from pending map
    PendingMap::iterator it = my_pending.find(txid);
    if (it != my_pending.end()) {
        CMPPending *p_pending = &(it->second);
        strTXText = "*** THIS TRANSACTION IS UNCONFIRMED ***\n" + p_pending->desc;
    } else {
        // grab details usual way
        int pop = populateRPCTransactionObject(txid, &txobj, "");
        if (0<=pop) {
            strTXText = write_string(Value(txobj), true);
            // manipulate for STO if needed
            size_t pos = strTXText.find("Send To Owners");
            if (pos!=std::string::npos) {
                Array receiveArray;
                uint64_t tmpAmount = 0;
                uint64_t tmpSTOFee = 0;
                s_stolistdb->getRecipients(txid, "", &receiveArray, &tmpAmount, &tmpSTOFee);
                txobj.push_back(Pair("recipients", receiveArray));
                //rewrite string
                strTXText = write_string(Value(txobj), true);
            }
        }
    }
    if (!strTXText.empty()) {
        PopulateSimpleDialog(strTXText, "Transaction Information", "Transaction Information");
    }
}

void TXHistoryDialog::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    borrowedColumnResizingFixer->stretchColumnWidth(5);
}

std::string TXHistoryDialog::shrinkTxType(int txType, bool *fundsMoved)
{
    string displayType = "Unknown";
    switch (txType) {
        case MSC_TYPE_SIMPLE_SEND: displayType = "Send"; break;
        case MSC_TYPE_RESTRICTED_SEND: displayType = "Rest. Send"; break;
        case MSC_TYPE_SEND_TO_OWNERS: displayType = "Send To Owners"; break;
        case MSC_TYPE_SAVINGS_MARK: displayType = "Mark Savings"; *fundsMoved = false; break;
        case MSC_TYPE_SAVINGS_COMPROMISED: ; displayType = "Lock Savings"; break;
        case MSC_TYPE_RATELIMITED_MARK: displayType = "Rate Limit"; break;
        case MSC_TYPE_AUTOMATIC_DISPENSARY: displayType = "Auto Dispense"; break;
        case MSC_TYPE_TRADE_OFFER: displayType = "DEx Trade"; *fundsMoved = false; break;
        case MSC_TYPE_METADEX: displayType = "MetaDEx Trade"; *fundsMoved = false; break;
        case MSC_TYPE_ACCEPT_OFFER_BTC: displayType = "DEx Accept"; *fundsMoved = false; break;
        case MSC_TYPE_CREATE_PROPERTY_FIXED: displayType = "Create Property"; break;
        case MSC_TYPE_CREATE_PROPERTY_VARIABLE: displayType = "Create Property"; break;
        case MSC_TYPE_PROMOTE_PROPERTY: displayType = "Promo Property"; break;
        case MSC_TYPE_CLOSE_CROWDSALE: displayType = "Close Crowdsale"; break;
        case MSC_TYPE_CREATE_PROPERTY_MANUAL: displayType = "Create Property"; break;
        case MSC_TYPE_GRANT_PROPERTY_TOKENS: displayType = "Grant Tokens"; break;
        case MSC_TYPE_REVOKE_PROPERTY_TOKENS: displayType = "Revoke Tokens"; break;
        case MSC_TYPE_CHANGE_ISSUER_ADDRESS: displayType = "Change Issuer"; *fundsMoved = false; break;
    }
    return displayType;
}
