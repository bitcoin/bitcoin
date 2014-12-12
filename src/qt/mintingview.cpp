#include "mintingview.h"
#include "mintingfilterproxy.h"
#include "transactionrecord.h"
#include "mintingtablemodel.h"
#include "walletmodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "csvmodelwriter.h"


#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QTableView>
#include <QScrollBar>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QMessageBox>
#include <QMenu>

MintingView::MintingView(QWidget *parent) :
    QWidget(parent), model(0), mintingView(0)
{
    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->setContentsMargins(0,0,0,0);

    QString legendBoxStyle = "background-color: rgb(%1,%2,%3); border: 1px solid black;";

    QLabel *youngColor = new QLabel(" ");
    youngColor->setMaximumHeight(15);
    youngColor->setMaximumWidth(10);
    youngColor->setStyleSheet(legendBoxStyle.arg(COLOR_MINT_YOUNG.red()).arg(COLOR_MINT_YOUNG.green()).arg(COLOR_MINT_YOUNG.blue()));
    QLabel *youngLegend = new QLabel(tr("transaction is too young"));
    youngLegend->setContentsMargins(5,0,15,0);

    QLabel *matureColor = new QLabel(" ");
    matureColor->setMaximumHeight(15);
    matureColor->setMaximumWidth(10);
    matureColor->setStyleSheet(legendBoxStyle.arg(COLOR_MINT_MATURE.red()).arg(COLOR_MINT_MATURE.green()).arg(COLOR_MINT_MATURE.blue()));
    QLabel *matureLegend = new QLabel(tr("transaction is mature"));
    matureLegend->setContentsMargins(5,0,15,0);

    QLabel *oldColor = new QLabel(" ");
    oldColor->setMaximumHeight(15);
    oldColor->setMaximumWidth(10);
    oldColor->setStyleSheet(legendBoxStyle.arg(COLOR_MINT_OLD.red()).arg(COLOR_MINT_OLD.green()).arg(COLOR_MINT_OLD.blue()));
    QLabel *oldLegend = new QLabel(tr("transaction has reached maximum probability"));
    oldLegend->setContentsMargins(5,0,15,0);

    QHBoxLayout *legendLayout = new QHBoxLayout();
    legendLayout->setContentsMargins(10,10,0,0);
    legendLayout->addWidget(youngColor);
    legendLayout->addWidget(youngLegend);
    legendLayout->addWidget(matureColor);
    legendLayout->addWidget(matureLegend);
    legendLayout->addWidget(oldColor);
    legendLayout->addWidget(oldLegend);
    legendLayout->insertStretch(-1);

    QLabel *mintingLabel = new QLabel(tr("Display minting probability within : "));
    mintingCombo = new QComboBox();
    mintingCombo->addItem(tr("10 min"), Minting10min);
    mintingCombo->addItem(tr("24 hours"), Minting1day);
    mintingCombo->addItem(tr("7 days"), Minting7days);
    mintingCombo->addItem(tr("30 days"), Minting30days);
    mintingCombo->addItem(tr("60 days"), Minting60days);
    mintingCombo->addItem(tr("90 days"), Minting90days);
    mintingCombo->setFixedWidth(120);


    hlayout->insertStretch(0);
    hlayout->addWidget(mintingLabel);
    hlayout->addWidget(mintingCombo);

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(0,0,0,0);
    vlayout->setSpacing(0);

    QTableView *view = new QTableView(this);
    vlayout->addLayout(hlayout);
    vlayout->addWidget(view);
    vlayout->addLayout(legendLayout);

    vlayout->setSpacing(0);
    int width = view->verticalScrollBar()->sizeHint().width();
    // Cover scroll bar width with spacing
#ifdef Q_WS_MAC
    hlayout->addSpacing(width+2);
#else
    hlayout->addSpacing(width);
#endif
    // Always show scroll bar
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    view->setTabKeyNavigation(false);
    view->setContextMenuPolicy(Qt::CustomContextMenu);

    mintingView = view;

    connect(mintingCombo, SIGNAL(activated(int)), this, SLOT(chooseMintingInterval(int)));

    // Actions
    QAction *copyTxIDAction = new QAction(tr("Copy transaction ID of input"), this);
    QAction *copyAddressAction = new QAction(tr("Copy address of input"), this);
    QAction *showHideAddressAction = new QAction(tr("Show/hide 'Address' column"), this);
    QAction *showHideTxIDAction = new QAction(tr("Show/hide 'Transaction' column"), this);

    contextMenu = new QMenu();
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyTxIDAction);
    contextMenu->addAction(showHideAddressAction);
    contextMenu->addAction(showHideTxIDAction);

    connect(copyAddressAction, SIGNAL(triggered()), this, SLOT(copyAddress()));
    connect(copyTxIDAction, SIGNAL(triggered()), this, SLOT(copyTxID()));
    connect(showHideAddressAction, SIGNAL(triggered()), this, SLOT(showHideAddress()));
    connect(showHideTxIDAction, SIGNAL(triggered()), this, SLOT(showHideTxID()));

    connect(view, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));
}


void MintingView::setModel(WalletModel *model)
{
    this->model = model;
    if(model)
    {
        mintingProxyModel = new MintingFilterProxy(this);
        mintingProxyModel->setSourceModel(model->getMintingTableModel());
        mintingProxyModel->setDynamicSortFilter(true);
        mintingProxyModel->setSortRole(Qt::EditRole);
        model->getMintingTableModel()->setMintingProxyModel(mintingProxyModel);

        mintingView->setModel(mintingProxyModel);
        mintingView->setAlternatingRowColors(true);
        mintingView->setSelectionBehavior(QAbstractItemView::SelectRows);
        mintingView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        mintingView->setSortingEnabled(true);
        mintingView->sortByColumn(MintingTableModel::CoinDay, Qt::DescendingOrder);
        mintingView->verticalHeader()->hide();

        mintingView->horizontalHeader()->resizeSection(
                MintingTableModel::Age, 60);
        mintingView->horizontalHeader()->resizeSection(
                MintingTableModel::Balance, 80);
        mintingView->horizontalHeader()->resizeSection(
                MintingTableModel::CoinDay,60);
        mintingView->horizontalHeader()->resizeSection(
                MintingTableModel::MintProbability, 105);
#if QT_VERSION < 0x050000
        mintingView->horizontalHeader()->setResizeMode(
                MintingTableModel::MintReward, QHeaderView::Stretch);
#else
        mintingView->horizontalHeader()->setSectionResizeMode(
                MintingTableModel::MintReward, QHeaderView::Stretch);
#endif
        mintingView->horizontalHeader()->resizeSection(
            MintingTableModel::Address, 245);
        mintingView->horizontalHeader()->resizeSection(
            MintingTableModel::TxHash, 75);
    }
}

void MintingView::chooseMintingInterval(int idx)
{
    int interval = 10;
    switch(mintingCombo->itemData(idx).toInt())
    {
        case Minting10min:
            interval = 10;
            break;
        case Minting1day:
            interval = 60*24;
            break;
        case Minting7days:
            interval = 60*24*7;
            break;
        case Minting30days:
            interval = 60*24*30;
            break;
        case Minting60days:
            interval = 60*24*60;
            break;
        case Minting90days:
            interval = 60*24*90;
            break;
    }
    model->getMintingTableModel()->setMintingInterval(interval);
    mintingProxyModel->invalidate();
}

void MintingView::exportClicked()
{
    // CSV is currently the only supported format
    QString filename = GUIUtil::getSaveFileName(
            this,
            tr("Export Minting Data"), QString(),
            tr("Comma separated file (*.csv)"));

    if (filename.isNull()) return;

    CSVModelWriter writer(filename);

    // name, column, role
    writer.setModel(mintingProxyModel);
    writer.addColumn(tr("Address"),MintingTableModel::Address,0);
    writer.addColumn(tr("Transaction"),MintingTableModel::TxHash,0);
    writer.addColumn(tr("Age"), MintingTableModel::Age,0);
    writer.addColumn(tr("CoinDay"), MintingTableModel::CoinDay,0);
    writer.addColumn(tr("Balance"), MintingTableModel::Balance,0);
    writer.addColumn(tr("MintingProbability"), MintingTableModel::MintProbability,0);
    writer.addColumn(tr("MintingReward"), MintingTableModel::MintReward,0);

    if(!writer.write())
    {
        QMessageBox::critical(this, tr("Error exporting"), tr("Could not write to file %1.").arg(filename),
                              QMessageBox::Abort, QMessageBox::Abort);
    }
}

void MintingView::copyTxID()
{
    GUIUtil::copyEntryData(mintingView, MintingTableModel::TxHash, 0);
}

void MintingView::copyAddress()
{
    GUIUtil::copyEntryData(mintingView, MintingTableModel::Address, 0 );
}

void MintingView::showHideAddress()
{
    mintingView->horizontalHeader()->setSectionHidden(MintingTableModel::Address, 
        !(mintingView->horizontalHeader()->isSectionHidden(MintingTableModel::Address)));
}

void MintingView::showHideTxID()
{
    mintingView->horizontalHeader()->setSectionHidden(MintingTableModel::TxHash, 
        !(mintingView->horizontalHeader()->isSectionHidden(MintingTableModel::TxHash)));
}

void MintingView::contextualMenu(const QPoint &point)
{
    QModelIndex index = mintingView->indexAt(point);
    if(index.isValid())
    {
        contextMenu->exec(QCursor::pos());
    }
}