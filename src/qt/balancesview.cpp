// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "balancesview.h"

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
#include <QSignalMapper>
#include <QTableView>
#include <QUrl>
#include <QVBoxLayout>

BalancesView::BalancesView(QWidget *parent) :
    QWidget(parent), clientModel(0), walletModel(0), balancesView(0)
{
    // Build filter row
    setContentsMargins(0,0,0,0);

    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->setContentsMargins(0,0,0,0);
#ifdef Q_OS_MAC
    hlayout->setSpacing(5);
    hlayout->addSpacing(26);
#else
    hlayout->setSpacing(0);
    hlayout->addSpacing(23);
#endif
    hlayout->addStretch();
    // property ID selector
    propSelLabel = new QLabel("Show Balances For: ");
    hlayout->addWidget(propSelLabel);

    propSelectorWidget = new QComboBox(this);
#ifdef Q_OS_MAC
    propSelectorWidget->setFixedWidth(301);
#else
    propSelectorWidget->setFixedWidth(300);
#endif
    //add the selector to the layout
    hlayout->addWidget(propSelectorWidget);

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(0,0,0,0);
    vlayout->setSpacing(0);

    //populate
    //prep matrix
    const int numRows = 3000;
    const int numColumns = 3;
    uint matrix[numRows][numColumns];
    MatrixModel *mmp = NULL;
    QTableView *view = NULL;
    //create matrix
    for (int i = 0; i < numRows; ++i)
         for (int j = 0; j < numColumns; ++j)
              matrix[i][j] = (i+1) * (j+1);
    //create a model which adapts the data (the matrix) to the view.
    mmp = new MatrixModel(numRows, numColumns, (uint*)matrix, 2147483646);  //id for all (summary)
    view = new QTableView(this);
    view->setModel(mmp);
    // adjust sizing - redone to allow user to adjust all columns
    borrowedColumnResizingFixer = new GUIUtil::TableViewLastColumnResizingFixer(view,100,100);
    // note neither resizetocontents or stretch allow user to adjust - go interactive then manually set widths
    #if QT_VERSION < 0x050000
       view->horizontalHeader()->setResizeMode(0, QHeaderView::Interactive);
       view->horizontalHeader()->setResizeMode(1, QHeaderView::Interactive);
       view->horizontalHeader()->setResizeMode(2, QHeaderView::Interactive);
       view->horizontalHeader()->setResizeMode(3, QHeaderView::Interactive);
    #else
       view->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
       view->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
       view->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);
       view->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive);
    #endif
    // whilst we can't use QHeaderView::ResizeToContents for auto we can use resizeColumnToContents programatically
    view->resizeColumnToContents(0);
    view->resizeColumnToContents(2);
    view->resizeColumnToContents(3);
    borrowedColumnResizingFixer->stretchColumnWidth(1);
    view->verticalHeader()->setVisible(false);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    vlayout->addLayout(hlayout);
    vlayout->addWidget(view);
    vlayout->setSpacing(0);
    int width = view->verticalScrollBar()->sizeHint().width();
    // Cover scroll bar width with spacing
#ifdef Q_OS_MAC
    hlayout->addSpacing(width+2);
#else
    hlayout->addSpacing(width);
#endif
    // Always show scroll bar
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    view->setTabKeyNavigation(false);
    view->setContextMenuPolicy(Qt::CustomContextMenu);

    balancesView = view;

    // Actions
    QAction *balancesCopyAddressAction = new QAction(tr("Copy address"), this);
    QAction *balancesCopyLabelAction = new QAction(tr("Copy label"), this);
    QAction *balancesCopyAmountAction = new QAction(tr("Copy amount"), this);

    contextMenu = new QMenu();
    contextMenu->addAction(balancesCopyAddressAction);
    contextMenu->addAction(balancesCopyLabelAction);
    contextMenu->addAction(balancesCopyAmountAction);

    mapperThirdPartyTxUrls = new QSignalMapper(this);

    // Connect actions
    connect(propSelectorWidget, SIGNAL(activated(int)), this, SLOT(propSelectorChanged(int)));

    connect(balancesCopyAddressAction, SIGNAL(triggered()), this, SLOT(balancesCopyAddress()));
    connect(balancesCopyLabelAction, SIGNAL(triggered()), this, SLOT(balancesCopyLabel()));
    connect(balancesCopyAmountAction, SIGNAL(triggered()), this, SLOT(balancesCopyAmount()));
    UpdatePropSelector();
}

void BalancesView::setClientModel(ClientModel *model)
{
    if (model != NULL) {
        this->clientModel = model;
        connect(model, SIGNAL(refreshOmniState()), this, SLOT(balancesUpdated()));
    }
}

void BalancesView::setWalletModel(WalletModel *model)
{
    if (model != NULL) {
        this->walletModel = model;
        connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64)), this, SLOT(balancesUpdated()));
    }
}

void BalancesView::UpdatePropSelector()
{
    QString spId = propSelectorWidget->itemData(propSelectorWidget->currentIndex()).toString();
    propSelectorWidget->clear();
    propSelectorWidget->addItem("Wallet Totals (Summary)","2147483646"); //use last possible ID for summary for now
    // trigger update of global balances
    set_wallet_totals();
    // populate property selector
    for (unsigned int propertyId = 1; propertyId<100000; propertyId++)
    {
        if ((global_balance_money_maineco[propertyId] > 0) || (global_balance_reserved_maineco[propertyId] > 0))
        {
            string spName;
            spName = getPropertyName(propertyId).c_str();
            if(spName.size()>20) spName=spName.substr(0,20)+"...";
            string spId = static_cast<ostringstream*>( &(ostringstream() << propertyId) )->str();
            spName += " (#" + spId + ")";
            propSelectorWidget->addItem(spName.c_str(),spId.c_str());
        }
    }
    for (unsigned int propertyId = 1; propertyId<100000; propertyId++)
    {
        if ((global_balance_money_testeco[propertyId] > 0) || (global_balance_reserved_testeco[propertyId] > 0))
        {
            string spName;
            spName = getPropertyName(propertyId+2147483647).c_str();
            if(spName.size()>20) spName=spName.substr(0,20)+"...";
            string spId = static_cast<ostringstream*>( &(ostringstream() << propertyId+2147483647) )->str();
            spName += " (#" + spId + ")";
            propSelectorWidget->addItem(spName.c_str(),spId.c_str());
        }
    }
    int propIdx = propSelectorWidget->findData(spId);
    if (propIdx != -1) { propSelectorWidget->setCurrentIndex(propIdx); }
}

void BalancesView::UpdateBalances()
{
    propSelectorChanged(0);
    UpdatePropSelector();
}

void BalancesView::propSelectorChanged(int idx)
{
    QString spId = propSelectorWidget->itemData(propSelectorWidget->currentIndex()).toString();
    unsigned int propertyId = spId.toUInt();
    //repopulate with new selected balances
    //prep matrix
    const int numRows = 3000;
    const int numColumns = 3;
    uint matrix[numRows][numColumns];
    MatrixModel *mmp = NULL;
    //create matrix
    for (int i = 0; i < numRows; ++i)
         for (int j = 0; j < numColumns; ++j)
              matrix[i][j] = (i+1) * (j+1);
    //create a model which adapts the data (the matrix) to the view.
    mmp = new MatrixModel(numRows, numColumns, (uint*)matrix, propertyId);
    balancesView->setModel(mmp);
}

void BalancesView::contextualMenu(const QPoint &point)
{
    QModelIndex index = balancesView->indexAt(point);
    if(index.isValid())
    {
//        contextMenu->exec(QCursor::pos());
    }
}

void BalancesView::balancesCopyAddress()
{
//    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::AddressRole);
}

void BalancesView::balancesCopyLabel()
{
//    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::LabelRole);
}

void BalancesView::balancesCopyAmount()
{
//    GUIUtil::copyEntryData(transactionView, 0, TransactionTableModel::FormattedAmountRole);
}

void BalancesView::balancesUpdated()
{
    UpdateBalances();
}

// We override the virtual resizeEvent of the QWidget to adjust tables column
// sizes as the tables width is proportional to the dialogs width.
void BalancesView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    borrowedColumnResizingFixer->stretchColumnWidth(1);
}

