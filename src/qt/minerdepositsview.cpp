// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "minerdepositsview.h"

#include "addresstablemodel.h"
#include "bitcreditunits.h"
#include "csvmodelwriter.h"
#include "editaddressdialog.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "transactiondescdialog.h"
#include "transactionfilterproxy.h"
#include "transactionrecord.h"
#include "transactiontablemodel.h"
#include "walletmodel.h"

#include "ui_interface.h"

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
#include <QMessageBox>

MinerDepositsView::MinerDepositsView(QWidget *parent) :
    QWidget(parent), bitcredit_model(0), deposit_model(0), transactionProxyModel(0),
    minerDepositView(0)
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

    dateWidget = new QComboBox(this);
#ifdef Q_OS_MAC
    dateWidget->setFixedWidth(121);
#else
    dateWidget->setFixedWidth(120);
#endif
    dateWidget->addItem(tr("All"), All);
    dateWidget->addItem(tr("Today"), Today);
    dateWidget->addItem(tr("This week"), ThisWeek);
    dateWidget->addItem(tr("This month"), ThisMonth);
    dateWidget->addItem(tr("Last month"), LastMonth);
    dateWidget->addItem(tr("This year"), ThisYear);
    dateWidget->addItem(tr("Range..."), Range);
    hlayout->addWidget(dateWidget);

    typeWidget = new QComboBox(this);
#ifdef Q_OS_MAC
    typeWidget->setFixedWidth(121);
#else
    typeWidget->setFixedWidth(120);
#endif

    typeWidget->addItem(tr("All"), Bitcredit_TransactionFilterProxy::ALL_TYPES);
    typeWidget->addItem(tr("Deposit"), Bitcredit_TransactionFilterProxy::TYPE(Bitcredit_TransactionRecord::Deposit));
    typeWidget->addItem(tr("Deposit change"), Bitcredit_TransactionFilterProxy::TYPE(Bitcredit_TransactionRecord::DepositChange));

    hlayout->addWidget(typeWidget);

    addressWidget = new QLineEdit(this);
#if QT_VERSION >= 0x040700
    addressWidget->setPlaceholderText(tr("Enter address or label to search"));
#endif
    hlayout->addWidget(addressWidget);

    amountWidget = new QLineEdit(this);
#if QT_VERSION >= 0x040700
    amountWidget->setPlaceholderText(tr("Min amount"));
#endif
#ifdef Q_OS_MAC
    amountWidget->setFixedWidth(97);
#else
    amountWidget->setFixedWidth(100);
#endif
    amountWidget->setValidator(new QDoubleValidator(0, 1e20, 8, this));
    hlayout->addWidget(amountWidget);

    QVBoxLayout *vlayout = new QVBoxLayout(this);
    vlayout->setContentsMargins(0,0,0,0);
    vlayout->setSpacing(0);

    QTableView *view = new QTableView(this);
    vlayout->addLayout(hlayout);
    vlayout->addWidget(createDateRangeWidget());
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

    minerDepositView = view;

    // Actions
    QAction *copyAddressAction = new QAction(tr("Copy address"), this);
    QAction *copyLabelAction = new QAction(tr("Copy label"), this);
    QAction *copyAmountAction = new QAction(tr("Copy amount"), this);
    QAction *copyTxIDAction = new QAction(tr("Copy transaction ID"), this);
    QAction *editLabelAction = new QAction(tr("Edit label"), this);
    QAction *showDetailsAction = new QAction(tr("Show transaction details"), this);
    QAction *deleteDepositAction = new QAction(tr("Delete deposit"), this);

    contextMenu = new QMenu();
    contextMenu->addAction(copyAddressAction);
    contextMenu->addAction(copyLabelAction);
    contextMenu->addAction(copyAmountAction);
    contextMenu->addAction(copyTxIDAction);
    contextMenu->addAction(editLabelAction);
    contextMenu->addAction(showDetailsAction);
    contextMenu->addAction(deleteDepositAction);

    mapperThirdPartyTxUrls = new QSignalMapper(this);

    // Connect actions
    connect(mapperThirdPartyTxUrls, SIGNAL(mapped(QString)), this, SLOT(openThirdPartyTxUrl(QString)));

    connect(dateWidget, SIGNAL(activated(int)), this, SLOT(chooseDate(int)));
    connect(typeWidget, SIGNAL(activated(int)), this, SLOT(chooseType(int)));
    connect(addressWidget, SIGNAL(textChanged(QString)), this, SLOT(changedPrefix(QString)));
    connect(amountWidget, SIGNAL(textChanged(QString)), this, SLOT(changedAmount(QString)));

    connect(view, SIGNAL(doubleClicked(QModelIndex)), this, SIGNAL(doubleClicked(QModelIndex)));
    connect(view, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));

    connect(copyAddressAction, SIGNAL(triggered()), this, SLOT(copyAddress()));
    connect(copyLabelAction, SIGNAL(triggered()), this, SLOT(copyLabel()));
    connect(copyAmountAction, SIGNAL(triggered()), this, SLOT(copyAmount()));
    connect(copyTxIDAction, SIGNAL(triggered()), this, SLOT(copyTxID()));
    connect(editLabelAction, SIGNAL(triggered()), this, SLOT(editLabel()));
    connect(showDetailsAction, SIGNAL(triggered()), this, SLOT(showDetails()));
    connect(deleteDepositAction, SIGNAL(triggered()), this, SLOT(deleteDeposit()));
}

void MinerDepositsView::setModel(Bitcredit_WalletModel *bitcredit_model, Bitcredit_WalletModel *deposit_model)
{
    this->bitcredit_model = bitcredit_model;
    this->deposit_model = deposit_model;
    if(deposit_model)
    {
        transactionProxyModel = new Bitcredit_TransactionFilterProxy(this);
        transactionProxyModel->setSourceModel(deposit_model->getTransactionTableModel());
        transactionProxyModel->setDynamicSortFilter(true);
        transactionProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
        transactionProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

        transactionProxyModel->setSortRole(Qt::EditRole);

        minerDepositView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        minerDepositView->setModel(transactionProxyModel);
        minerDepositView->setAlternatingRowColors(true);
        minerDepositView->setSelectionBehavior(QAbstractItemView::SelectRows);
        minerDepositView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        minerDepositView->setSortingEnabled(true);
        minerDepositView->sortByColumn(Bitcredit_TransactionTableModel::Status, Qt::DescendingOrder);
        minerDepositView->verticalHeader()->hide();

        minerDepositView->setColumnWidth(Bitcredit_TransactionTableModel::Status, STATUS_COLUMN_WIDTH);
        minerDepositView->setColumnWidth(Bitcredit_TransactionTableModel::Date, DATE_COLUMN_WIDTH);
        minerDepositView->setColumnWidth(Bitcredit_TransactionTableModel::Type, TYPE_COLUMN_WIDTH);
        minerDepositView->setColumnWidth(Bitcredit_TransactionTableModel::Amount, AMOUNT_MINIMUM_COLUMN_WIDTH);

        columnResizingFixer = new GUIUtil::TableViewLastColumnResizingFixer(minerDepositView, AMOUNT_MINIMUM_COLUMN_WIDTH, MINIMUM_COLUMN_WIDTH);

        if (deposit_model->getOptionsModel())
        {
            // Add third party transaction URLs to context menu
            QStringList listUrls = deposit_model->getOptionsModel()->getThirdPartyTxUrls().split("|", QString::SkipEmptyParts);
            for (int i = 0; i < listUrls.size(); ++i)
            {
                QString host = QUrl(listUrls[i].trimmed(), QUrl::StrictMode).host();
                if (!host.isEmpty())
                {
                    QAction *thirdPartyTxUrlAction = new QAction(host, this); // use host as menu item label
                    if (i == 0)
                        contextMenu->addSeparator();
                    contextMenu->addAction(thirdPartyTxUrlAction);
                    connect(thirdPartyTxUrlAction, SIGNAL(triggered()), mapperThirdPartyTxUrls, SLOT(map()));
                    mapperThirdPartyTxUrls->setMapping(thirdPartyTxUrlAction, listUrls[i].trimmed());
                }
            }
        }
    }
}

void MinerDepositsView::chooseDate(int idx)
{
    if(!transactionProxyModel)
        return;
    QDate current = QDate::currentDate();
    dateRangeWidget->setVisible(false);
    switch(dateWidget->itemData(idx).toInt())
    {
    case All:
        transactionProxyModel->setDateRange(
                Bitcredit_TransactionFilterProxy::MIN_DATE,
                Bitcredit_TransactionFilterProxy::MAX_DATE);
        break;
    case Today:
        transactionProxyModel->setDateRange(
                QDateTime(current),
                Bitcredit_TransactionFilterProxy::MAX_DATE);
        break;
    case ThisWeek: {
        // Find last Monday
        QDate startOfWeek = current.addDays(-(current.dayOfWeek()-1));
        transactionProxyModel->setDateRange(
                QDateTime(startOfWeek),
                Bitcredit_TransactionFilterProxy::MAX_DATE);

        } break;
    case ThisMonth:
        transactionProxyModel->setDateRange(
                QDateTime(QDate(current.year(), current.month(), 1)),
                Bitcredit_TransactionFilterProxy::MAX_DATE);
        break;
    case LastMonth:
        transactionProxyModel->setDateRange(
                QDateTime(QDate(current.year(), current.month()-1, 1)),
                QDateTime(QDate(current.year(), current.month(), 1)));
        break;
    case ThisYear:
        transactionProxyModel->setDateRange(
                QDateTime(QDate(current.year(), 1, 1)),
                Bitcredit_TransactionFilterProxy::MAX_DATE);
        break;
    case Range:
        dateRangeWidget->setVisible(true);
        dateRangeChanged();
        break;
    }
}

void MinerDepositsView::chooseType(int idx)
{
    if(!transactionProxyModel)
        return;
    transactionProxyModel->setTypeFilter(
        typeWidget->itemData(idx).toInt());
}

void MinerDepositsView::changedPrefix(const QString &prefix)
{
    if(!transactionProxyModel)
        return;
    transactionProxyModel->setAddressPrefix(prefix);
}

void MinerDepositsView::changedAmount(const QString &amount)
{
    if(!transactionProxyModel)
        return;
    qint64 amount_parsed = 0;
    if(BitcreditUnits::parse(deposit_model->getOptionsModel()->getDisplayUnit(), amount, &amount_parsed))
    {
        transactionProxyModel->setMinAmount(amount_parsed);
    }
    else
    {
        transactionProxyModel->setMinAmount(0);
    }
}

void MinerDepositsView::contextualMenu(const QPoint &point)
{
    QModelIndex index = minerDepositView->indexAt(point);
    if(index.isValid())
    {
        contextMenu->exec(QCursor::pos());
    }
}

void MinerDepositsView::copyAddress()
{
    GUIUtil::copyEntryData(minerDepositView, 0, Bitcredit_TransactionTableModel::AddressRole);
}

void MinerDepositsView::copyLabel()
{
    GUIUtil::copyEntryData(minerDepositView, 0, Bitcredit_TransactionTableModel::LabelRole);
}

void MinerDepositsView::copyAmount()
{
    GUIUtil::copyEntryData(minerDepositView, 0, Bitcredit_TransactionTableModel::FormattedDepositAmountRole);
}

void MinerDepositsView::copyTxID()
{
    GUIUtil::copyEntryData(minerDepositView, 0, Bitcredit_TransactionTableModel::TxIDRole);
}

void MinerDepositsView::editLabel()
{
    if(!minerDepositView->selectionModel() ||!deposit_model)
        return;
    QModelIndexList selection = minerDepositView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        Bitcredit_AddressTableModel *addressBook = deposit_model->getAddressTableModel();
        if(!addressBook)
            return;
        QString address = selection.at(0).data(Bitcredit_TransactionTableModel::AddressRole).toString();
        if(address.isEmpty())
        {
            // If this transaction has no associated address, exit
            return;
        }
        // Is address in address book? Address book can miss address when a transaction is
        // sent from outside the UI.
        int idx = addressBook->lookupAddress(address);
        if(idx != -1)
        {
            // Edit sending / receiving address
            QModelIndex modelIdx = addressBook->index(idx, 0, QModelIndex());
            // Determine type of address, launch appropriate editor dialog type
            QString type = modelIdx.data(Bitcredit_AddressTableModel::TypeRole).toString();

            Bitcredit_EditAddressDialog dlg(
                type == Bitcredit_AddressTableModel::Receive
                ? Bitcredit_EditAddressDialog::EditReceivingAddress
                : Bitcredit_EditAddressDialog::EditSendingAddress, this);
            dlg.setModel(addressBook);
            dlg.loadRow(idx);
            dlg.exec();
        }
        else
        {
            // Add sending address
            Bitcredit_EditAddressDialog dlg(Bitcredit_EditAddressDialog::NewSendingAddress,
                this);
            dlg.setModel(addressBook);
            dlg.setAddress(address);
            dlg.exec();
        }
    }
}

void MinerDepositsView::showDetails()
{
    if(!minerDepositView->selectionModel())
        return;
    QModelIndexList selection = minerDepositView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
        TransactionDescDialog dlg(selection.at(0));
        dlg.exec();
    }
}
void MinerDepositsView::deleteDeposit()
{
    if(!minerDepositView->selectionModel())
        return;
    QModelIndexList selection = minerDepositView->selectionModel()->selectedRows();
    if(!selection.isEmpty())
    {
    	QString questionString = tr("Are you sure you want to delete this deposit?");
        questionString.append(tr("<br /><br />Deleting the deposit does NOT remove the related coins but simply returns them to a spendable state."));
        QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm delete deposit"),
            questionString,
            QMessageBox::Yes | QMessageBox::Cancel,
            QMessageBox::Cancel);

        if(retval != QMessageBox::Yes)
        {
            return;
        }

        const QString hashString = selection.at(0).data(Bitcredit_TransactionTableModel::TxHashRole).toString();
        const uint256 hashTx = uint256(hashString.toStdString());

        deposit_model->wallet->EraseFromWallet(bitcredit_model->wallet, hashTx);
        deposit_model->checkBalanceChanged();
    }
}

void MinerDepositsView::openThirdPartyTxUrl(QString url)
{
    if(!minerDepositView || !minerDepositView->selectionModel())
        return;
    QModelIndexList selection = minerDepositView->selectionModel()->selectedRows(0);
    if(!selection.isEmpty())
         QDesktopServices::openUrl(QUrl::fromUserInput(url.replace("%s", selection.at(0).data(Bitcredit_TransactionTableModel::TxHashRole).toString())));
}

QWidget *MinerDepositsView::createDateRangeWidget()
{
    dateRangeWidget = new QFrame();
    dateRangeWidget->setFrameStyle(QFrame::Panel | QFrame::Raised);
    dateRangeWidget->setContentsMargins(1,1,1,1);
    QHBoxLayout *layout = new QHBoxLayout(dateRangeWidget);
    layout->setContentsMargins(0,0,0,0);
    layout->addSpacing(23);
    layout->addWidget(new QLabel(tr("Range:")));

    dateFrom = new QDateTimeEdit(this);
    dateFrom->setDisplayFormat("dd/MM/yy");
    dateFrom->setCalendarPopup(true);
    dateFrom->setMinimumWidth(100);
    dateFrom->setDate(QDate::currentDate().addDays(-7));
    layout->addWidget(dateFrom);
    layout->addWidget(new QLabel(tr("to")));

    dateTo = new QDateTimeEdit(this);
    dateTo->setDisplayFormat("dd/MM/yy");
    dateTo->setCalendarPopup(true);
    dateTo->setMinimumWidth(100);
    dateTo->setDate(QDate::currentDate());
    layout->addWidget(dateTo);
    layout->addStretch();

    // Hide by default
    dateRangeWidget->setVisible(false);

    // Notify on change
    connect(dateFrom, SIGNAL(dateChanged(QDate)), this, SLOT(dateRangeChanged()));
    connect(dateTo, SIGNAL(dateChanged(QDate)), this, SLOT(dateRangeChanged()));

    return dateRangeWidget;
}

void MinerDepositsView::dateRangeChanged()
{
    if(!transactionProxyModel)
        return;
    transactionProxyModel->setDateRange(
            QDateTime(dateFrom->date()),
            QDateTime(dateTo->date()).addDays(1));
}

void MinerDepositsView::focusTransaction(const QModelIndex &idx)
{
    if(!transactionProxyModel)
        return;
    QModelIndex targetIdx = transactionProxyModel->mapFromSource(idx);
    minerDepositView->scrollTo(targetIdx);
    minerDepositView->setCurrentIndex(targetIdx);
    minerDepositView->setFocus();
}

// We override the virtual resizeEvent of the QWidget to adjust tables column
// sizes as the tables width is proportional to the dialogs width.
void MinerDepositsView::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    columnResizingFixer->stretchColumnWidth(Bitcredit_TransactionTableModel::ToAddress);
}
