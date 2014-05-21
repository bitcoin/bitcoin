#include "mintingview.h"
#include "mintingfilterproxy.h"
#include "transactionrecord.h"
#include "mintingtablemodel.h"
#include "walletmodel.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QTableView>
#include <QScrollBar>


MintingView::MintingView(QWidget *parent) :
    QWidget(parent), model(0), mintingView(0)
{
    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->setContentsMargins(0,0,0,0);
#ifdef Q_WS_MAC
    hlayout->setSpacing(5);
    hlayout->addSpacing(26);
#else
    hlayout->setSpacing(0);
    hlayout->addSpacing(23);
#endif

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(0,0,0,0);
    vlayout->setSpacing(0);

    QTableView *view = new QTableView(this);
    vlayout->addLayout(hlayout);
    vlayout->addWidget(view);
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

}


void MintingView::setModel(WalletModel *model)
{
    this->model = model;
    if(model)
    {
        MintingFilterProxy *mintingProxyModel = new MintingFilterProxy(this);
        mintingProxyModel->setSourceModel(model->getMintingTableModel());
        mintingProxyModel->setDynamicSortFilter(true);
        mintingProxyModel->setSortRole(Qt::EditRole);

        mintingView->setModel(mintingProxyModel);
        mintingView->setAlternatingRowColors(true);
        mintingView->setSelectionBehavior(QAbstractItemView::SelectRows);
        mintingView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        mintingView->setSortingEnabled(true);
        mintingView->sortByColumn(MintingTableModel::CoinDay, Qt::DescendingOrder);
        mintingView->verticalHeader()->hide();

        mintingView->horizontalHeader()->setResizeMode(
                MintingTableModel::Address, QHeaderView::Stretch);
        mintingView->horizontalHeader()->resizeSection(
                MintingTableModel::Age, 120);
        mintingView->horizontalHeader()->resizeSection(
                MintingTableModel::Balance, 120);
        mintingView->horizontalHeader()->resizeSection(
                MintingTableModel::CoinDay,120);
        mintingView->horizontalHeader()->resizeSection(
                MintingTableModel::MintProbabilityPerDay, 120);
    }
}

void MintingView::exportClicked()
{
    // TODO
}
