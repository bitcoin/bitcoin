// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "bitcreditunits.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "transactionfilterproxy.h"
#include "transactiontablemodel.h"
#include "walletmodel.h"

#include <QAbstractItemDelegate>
#include <QPainter>

#define DECORATION_SIZE 64
#define NUM_ITEMS 3

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate(): QAbstractItemDelegate(), unit(BitcreditUnits::CRE)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        QRect mainRect = option.rect;
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(Bitcredit_TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(Bitcredit_TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(Bitcredit_TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);
        if(value.canConvert<QBrush>())
        {
            QBrush brush = qvariant_cast<QBrush>(value);
            foreground = brush.color();
        }

        painter->setPen(foreground);
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, address);

        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
        }
        else if(!confirmed)
        {
            foreground = COLOR_UNCONFIRMED;
        }
        else
        {
            foreground = option.palette.color(QPalette::Text);
        }
        painter->setPen(foreground);
        QString amountText = BitcreditUnits::formatWithUnit(unit, amount, true);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        painter->setPen(option.palette.color(QPalette::Text));
        painter->drawText(amountRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;

};
#include "overviewpage.moc"

OverviewPage::OverviewPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage),
    clientModel(0),
    bitcredit_model(0),
    deposit_model(0),
    currentBalance(-1),
    currentUnconfirmedBalance(-1),
    currentImmatureBalance(-1),
    currentPreparedDepositBalance(-1),
    currentInDepositBalance(-1),
    txdelegate(new TxViewDelegate()),
    filter(0)
{
    ui->setupUi(this);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));

    // init "out of sync" warning labels
    ui->labelWalletStatus->setText("(" + tr("out of sync") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("out of sync") + ")");

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);
}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        emit transactionClicked(filter->mapToSource(index));
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::setBalance(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance, qint64 preparedDepositBalance, qint64 inDepositBalance)
{
    int unit = bitcredit_model->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentPreparedDepositBalance = preparedDepositBalance;
    currentInDepositBalance = inDepositBalance;
    ui->labelBalance->setText(BitcreditUnits::formatWithUnit(unit, balance));
    ui->labelUnconfirmed->setText(BitcreditUnits::formatWithUnit(unit, unconfirmedBalance));
    ui->labelImmature->setText(BitcreditUnits::formatWithUnit(unit, immatureBalance));
    ui->labelPreparedDeposit->setText(BitcreditUnits::formatWithUnit(unit, preparedDepositBalance));
    ui->labelInDeposit->setText(BitcreditUnits::formatWithUnit(unit, inDepositBalance));
    ui->labelTotal->setText(BitcreditUnits::formatWithUnit(unit, balance + unconfirmedBalance + immatureBalance + preparedDepositBalance + inDepositBalance));

//    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
//    // for the non-mining users
//    bool showImmature = immatureBalance != 0;
//    ui->labelImmature->setVisible(showImmature);
//    ui->labelImmatureText->setVisible(showImmature);
//
//    // only show deposit balance if it's non-zero, so as not to complicate things
//    // for the non-mining users
//    bool showPreparedDeposit = preparedDepositBalance != 0;
//    ui->labelPreparedDeposit->setVisible(showPreparedDeposit);
//    ui->labelPreparedDepositText->setVisible(showPreparedDeposit);
//
//    // only show deposit balance if it's non-zero, so as not to complicate things
//    // for the non-mining users
//    bool showInDeposit = inDepositBalance != 0;
//    ui->labelInDeposit->setVisible(showInDeposit);
//    ui->labelInDepositText->setVisible(showInDeposit);
}

void OverviewPage::refreshBalance()
{
    //Find all prepared deposit transactions
    map<uint256, set<int> > mapPreparedDepositTxInPoints;
    deposit_model->wallet->PreparedDepositTxInPoints(mapPreparedDepositTxInPoints);

    setBalance(bitcredit_model->getBalance(mapPreparedDepositTxInPoints), bitcredit_model->getUnconfirmedBalance(mapPreparedDepositTxInPoints), bitcredit_model->getImmatureBalance(mapPreparedDepositTxInPoints), bitcredit_model->getPreparedDepositBalance(), bitcredit_model->getInDepositBalance());
}

void OverviewPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {
        // Show warning if this is a prerelease version
        connect(model, SIGNAL(alertsChanged(QString)), this, SLOT(updateAlerts(QString)));
        updateAlerts(model->getStatusBarWarnings());
    }
}

void OverviewPage::setWalletModel(Bitcredit_WalletModel *bitcredit_model, Bitcredit_WalletModel *deposit_model)
{
    this->bitcredit_model = bitcredit_model;
    this->deposit_model = deposit_model;

    if(bitcredit_model && bitcredit_model->getOptionsModel())
    {
        // Set up transaction list
        filter = new Bitcredit_TransactionFilterProxy();
        filter->setSourceModel(bitcredit_model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(Bitcredit_TransactionTableModel::Status, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter);
        ui->listTransactions->setModelColumn(Bitcredit_TransactionTableModel::ToAddress);

        refreshBalance();

        // Keep up to date with  wallets
        connect(bitcredit_model, SIGNAL(balanceChanged(qint64, qint64, qint64, qint64, qint64)), this, SLOT(setBalance(qint64, qint64, qint64, qint64, qint64)));
        connect(deposit_model, SIGNAL(balanceChanged(qint64, qint64, qint64, qint64, qint64)), this, SLOT(refreshBalance()));

        connect(bitcredit_model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
    }

    // update the display unit, to not use the default ("CRE")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if(bitcredit_model && bitcredit_model->getOptionsModel())
    {
        if(currentBalance != -1)
            setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance, currentPreparedDepositBalance, currentInDepositBalance);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = bitcredit_model->getOptionsModel()->getDisplayUnit();

        ui->listTransactions->update();
    }
}

void OverviewPage::updateAlerts(const QString &warnings)
{
    this->ui->labelAlerts->setVisible(!warnings.isEmpty());
    this->ui->labelAlerts->setText(warnings);
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}
