// Copyright (c) 2011-2013 The Bitcoin developers // Distributed under the MIT/X11 software license, see the accompanying // file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "balancesdialog.h"
#include "ui_balancesdialog.h"

#include "addresstablemodel.h"
#include "bitcoinunits.h"
#include "clientmodel.h"
#include "csvmodelwriter.h"
#include "editaddressdialog.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "transactiondescdialog.h"
#include "transactionfilterproxy.h"
#include "transactionrecord.h"
#include "transactiontablemodel.h"
#include "walletmodel.h"
#include "wallet.h"

#include "ui_interface.h"

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
using namespace json_spirit; // since now using Array in mastercore.h this needs to come first

#include "mastercore.h"
using namespace mastercore;

// potentially overzealous using here
using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace leveldb;
// end potentially overzealous using

#include "mastercore_dex.h"
#include "mastercore_tx.h"
#include "mastercore_sp.h"

#include <QComboBox>
#include <QDateTimeEdit>
#include <QDesktopServices>
#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPoint>
#include <QScrollBar>
#include <QTableView>
#include <QUrl>
#include <QVBoxLayout>

BalancesDialog::BalancesDialog(QWidget *parent) :
    QDialog(parent), ui(new Ui::balancesDialog), clientModel(0), walletModel(0)
{
    // setup
    ui->setupUi(this);
    ui->balancesTable->setColumnCount(4);
    ui->balancesTable->setHorizontalHeaderItem(0, new QTableWidgetItem("Property ID"));
    ui->balancesTable->setHorizontalHeaderItem(1, new QTableWidgetItem("Property Name"));
    ui->balancesTable->setHorizontalHeaderItem(2, new QTableWidgetItem("Reserved"));
    ui->balancesTable->setHorizontalHeaderItem(3, new QTableWidgetItem("Available"));
    borrowedColumnResizingFixer = new GUIUtil::TableViewLastColumnResizingFixer(ui->balancesTable,100,100);
    // note neither resizetocontents or stretch allow user to adjust - go interactive then manually set widths
    #if QT_VERSION < 0x050000
       ui->balancesTable->horizontalHeader()->setResizeMode(0, QHeaderView::Interactive);
       ui->balancesTable->horizontalHeader()->setResizeMode(1, QHeaderView::Interactive);
       ui->balancesTable->horizontalHeader()->setResizeMode(2, QHeaderView::Interactive);
       ui->balancesTable->horizontalHeader()->setResizeMode(3, QHeaderView::Interactive);
    #else
       ui->balancesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
       ui->balancesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
       ui->balancesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);
       ui->balancesTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive);
    #endif
    ui->balancesTable->setAlternatingRowColors(true);

    // do an initial population
    UpdatePropSelector();
    PopulateBalances(2147483646); // 2147483646 = summary (last possible ID for test eco props)

    // initial resizing
    ui->balancesTable->resizeColumnToContents(0);
    ui->balancesTable->resizeColumnToContents(2);
    ui->balancesTable->resizeColumnToContents(3);
    borrowedColumnResizingFixer->stretchColumnWidth(1);
    ui->balancesTable->verticalHeader()->setVisible(false);
    ui->balancesTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->balancesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->balancesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->balancesTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    ui->balancesTable->setTabKeyNavigation(false);
    ui->balancesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->balancesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Actions
    QAction *balancesCopyIDAction = new QAction(tr("Copy property ID"), this);
    QAction *balancesCopyNameAction = new QAction(tr("Copy property name"), this);
    QAction *balancesCopyAddressAction = new QAction(tr("Copy address"), this);
    QAction *balancesCopyLabelAction = new QAction(tr("Copy label"), this);
    QAction *balancesCopyReservedAmountAction = new QAction(tr("Copy reserved amount"), this);
    QAction *balancesCopyAvailableAmountAction = new QAction(tr("Copy available amount"), this);

    contextMenu = new QMenu();
    contextMenu->addAction(balancesCopyLabelAction);
    contextMenu->addAction(balancesCopyAddressAction);
    contextMenu->addAction(balancesCopyReservedAmountAction);
    contextMenu->addAction(balancesCopyAvailableAmountAction);
    contextMenuSummary = new QMenu();
    contextMenuSummary->addAction(balancesCopyIDAction);
    contextMenuSummary->addAction(balancesCopyNameAction);
    contextMenuSummary->addAction(balancesCopyReservedAmountAction);
    contextMenuSummary->addAction(balancesCopyAvailableAmountAction);

    // Connect actions
    connect(ui->balancesTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));
    connect(ui->propSelectorWidget, SIGNAL(activated(int)), this, SLOT(propSelectorChanged()));
    connect(balancesCopyIDAction, SIGNAL(triggered()), this, SLOT(balancesCopyCol0()));
    connect(balancesCopyNameAction, SIGNAL(triggered()), this, SLOT(balancesCopyCol1()));
    connect(balancesCopyLabelAction, SIGNAL(triggered()), this, SLOT(balancesCopyCol0()));
    connect(balancesCopyAddressAction, SIGNAL(triggered()), this, SLOT(balancesCopyCol1()));
    connect(balancesCopyReservedAmountAction, SIGNAL(triggered()), this, SLOT(balancesCopyCol2()));
    connect(balancesCopyAvailableAmountAction, SIGNAL(triggered()), this, SLOT(balancesCopyCol3()));
}

void BalancesDialog::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if (model != NULL) {
        connect(model, SIGNAL(refreshOmniState()), this, SLOT(balancesUpdated()));
    }
}

void BalancesDialog::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if (model != NULL) {
        connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(balancesUpdated()));
    }
}

void BalancesDialog::UpdatePropSelector()
{
    set_wallet_totals();
    QString spId = ui->propSelectorWidget->itemData(ui->propSelectorWidget->currentIndex()).toString();
    ui->propSelectorWidget->clear();
    ui->propSelectorWidget->addItem("Wallet Totals (Summary)","2147483646"); //use last possible ID for summary for now
    // populate property selector
    unsigned int nextPropIdMainEco = GetNextPropertyId(true);  // these allow us to end the for loop at the highest existing
    unsigned int nextPropIdTestEco = GetNextPropertyId(false); // property ID rather than a fixed value like 100000 (optimization)
    for (unsigned int propertyId = 1; propertyId < nextPropIdMainEco; propertyId++) {
        if ((global_balance_money_maineco[propertyId] > 0) || (global_balance_reserved_maineco[propertyId] > 0)) {
            string spName;
            spName = getPropertyName(propertyId).c_str();
            if(spName.size()>20) spName=spName.substr(0,20)+"...";
            string spId = static_cast<ostringstream*>( &(ostringstream() << propertyId) )->str();
            spName += " (#" + spId + ")";
            ui->propSelectorWidget->addItem(spName.c_str(),spId.c_str());
        }
    }
    for (unsigned int propertyId = 2147483647; propertyId < nextPropIdTestEco; propertyId++) {
        if ((global_balance_money_testeco[propertyId-2147483647] > 0) || (global_balance_reserved_testeco[propertyId-2147483647] > 0)) {
            string spName;
            spName = getPropertyName(propertyId).c_str();
            if(spName.size()>20) spName=spName.substr(0,20)+"...";
            string spId = static_cast<ostringstream*>( &(ostringstream() << propertyId) )->str();
            spName += " (#" + spId + ")";
            ui->propSelectorWidget->addItem(spName.c_str(),spId.c_str());
        }
    }
    int propIdx = ui->propSelectorWidget->findData(spId);
    if (propIdx != -1) { ui->propSelectorWidget->setCurrentIndex(propIdx); }
}

void BalancesDialog::AddRow(string label, string address, string reserved, string available)
{
    int workingRow = ui->balancesTable->rowCount();
    ui->balancesTable->insertRow(workingRow);
    QTableWidgetItem *labelCell = new QTableWidgetItem(QString::fromStdString(label));
    QTableWidgetItem *addressCell = new QTableWidgetItem(QString::fromStdString(address));
    QTableWidgetItem *reservedCell = new QTableWidgetItem(QString::fromStdString(reserved));
    QTableWidgetItem *availableCell = new QTableWidgetItem(QString::fromStdString(available));
    labelCell->setTextAlignment(Qt::AlignLeft + Qt::AlignVCenter);
    addressCell->setTextAlignment(Qt::AlignLeft + Qt::AlignVCenter);
    reservedCell->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
    availableCell->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
    ui->balancesTable->setItem(workingRow, 0, labelCell);
    ui->balancesTable->setItem(workingRow, 1, addressCell);
    ui->balancesTable->setItem(workingRow, 2, reservedCell);
    ui->balancesTable->setItem(workingRow, 3, availableCell);
}

void BalancesDialog::PopulateBalances(unsigned int propertyId)
{
    ui->balancesTable->setRowCount(0); // fresh slate (note this will automatically cleanup all existing QWidgetItems in the table)
    //are we summary?
    if(propertyId==2147483646) {
        ui->balancesTable->setHorizontalHeaderItem(0, new QTableWidgetItem("Property ID"));
        ui->balancesTable->setHorizontalHeaderItem(1, new QTableWidgetItem("Property Name"));
        unsigned int nextPropIdMainEco = GetNextPropertyId(true);
        unsigned int nextPropIdTestEco = GetNextPropertyId(false);
        for (unsigned int propertyId = 1; propertyId < nextPropIdMainEco; propertyId++) {
            if ((global_balance_money_maineco[propertyId] > 0) || (global_balance_reserved_maineco[propertyId] > 0)) {
                string spName = getPropertyName(propertyId).c_str();
                string spId = static_cast<ostringstream*>( &(ostringstream() << propertyId) )->str();
                string reserved;
                string available;
                if(isPropertyDivisible(propertyId)) {
                    reserved = FormatDivisibleMP(global_balance_reserved_maineco[propertyId]);
                    available = FormatDivisibleMP(global_balance_money_maineco[propertyId]);
                } else {
                    reserved = FormatIndivisibleMP(global_balance_reserved_maineco[propertyId]);
                    available = FormatIndivisibleMP(global_balance_money_maineco[propertyId]);
                }
                AddRow(spId,spName,reserved,available);
            }
        }
        for (unsigned int propertyId = 2147483647; propertyId < nextPropIdTestEco; propertyId++) {
            if ((global_balance_money_testeco[propertyId-2147483647] > 0) || (global_balance_reserved_testeco[propertyId-2147483647] > 0)) {
                string spName = getPropertyName(propertyId).c_str();
                string spId = static_cast<ostringstream*>( &(ostringstream() << propertyId) )->str();
                string reserved;
                string available;
                if(isPropertyDivisible(propertyId)) {
                    reserved = FormatDivisibleMP(global_balance_reserved_testeco[propertyId-2147483647]);
                    available = FormatDivisibleMP(global_balance_money_testeco[propertyId-2147483647]);
                } else {
                    reserved = FormatIndivisibleMP(global_balance_reserved_testeco[propertyId-2147483647]);
                    available = FormatIndivisibleMP(global_balance_money_testeco[propertyId-2147483647]);
                }
                AddRow(spId,spName,reserved,available);
            }
        }
    } else {
        ui->balancesTable->setHorizontalHeaderItem(0, new QTableWidgetItem("Label"));
        ui->balancesTable->setHorizontalHeaderItem(1, new QTableWidgetItem("Address"));
        LOCK(cs_tally);
        for(map<string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it)
        {
            string address = (my_it->first).c_str();
            unsigned int id;
            bool includeAddress=false;
            (my_it->second).init();
            while (0 != (id = (my_it->second).next())) {
                if(id==propertyId) { includeAddress=true; break; }
            }
            if (!includeAddress) continue; //ignore this address, has never transacted in this propertyId
            if (!IsMyAddress(address)) continue; //ignore this address, it's not ours
            int64_t available = getUserAvailableMPbalance(address, propertyId);
            int64_t reserved = getMPbalance(address, propertyId, METADEX_RESERVE);
            if (propertyId<3) {
                reserved += getMPbalance(address, propertyId, ACCEPT_RESERVE);
                reserved += getMPbalance(address, propertyId, SELLOFFER_RESERVE);
            }
            string reservedStr;
            string availableStr;
            if(isPropertyDivisible(propertyId)) {
                reservedStr = FormatDivisibleMP(reserved);
                availableStr = FormatDivisibleMP(available);
            } else {
                reservedStr = FormatIndivisibleMP(reserved);
                availableStr = FormatIndivisibleMP(available);
            }
            AddRow(getLabel(my_it->first),my_it->first,reservedStr,availableStr);
        }
    }
}

void BalancesDialog::propSelectorChanged()
{
    QString spId = ui->propSelectorWidget->itemData(ui->propSelectorWidget->currentIndex()).toString();
    unsigned int propertyId = spId.toUInt();
    PopulateBalances(propertyId);
}

void BalancesDialog::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->balancesTable->indexAt(point);
    if(index.isValid())
    {
        QString spId = ui->propSelectorWidget->itemData(ui->propSelectorWidget->currentIndex()).toString();
        unsigned int propertyId = spId.toUInt();
        if (propertyId == 2147483646) {
            contextMenuSummary->exec(QCursor::pos());
        } else {
            contextMenu->exec(QCursor::pos());
        }
    }
}

void BalancesDialog::balancesCopyCol0()
{
    GUIUtil::setClipboard(ui->balancesTable->item(ui->balancesTable->currentRow(),0)->text());
}

void BalancesDialog::balancesCopyCol1()
{
    GUIUtil::setClipboard(ui->balancesTable->item(ui->balancesTable->currentRow(),1)->text());
}

void BalancesDialog::balancesCopyCol2()
{
    GUIUtil::setClipboard(ui->balancesTable->item(ui->balancesTable->currentRow(),2)->text());
}

void BalancesDialog::balancesCopyCol3()
{
    GUIUtil::setClipboard(ui->balancesTable->item(ui->balancesTable->currentRow(),3)->text());
}

void BalancesDialog::balancesUpdated()
{
    UpdatePropSelector();
    propSelectorChanged(); // refresh the table with the currently selected property ID
}

// We override the virtual resizeEvent of the QWidget to adjust tables column
// sizes as the tables width is proportional to the dialogs width.
void BalancesDialog::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    borrowedColumnResizingFixer->stretchColumnWidth(1);
}

