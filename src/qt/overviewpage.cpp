// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "ravenunits.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "transactionfilterproxy.h"
#include "transactiontablemodel.h"
#include "assettablemodel.h"
#include "walletmodel.h"

#include <QAbstractItemDelegate>
#include <QPainter>
#include <validation.h>
#include <utiltime.h>

#define DECORATION_SIZE 54
#define NUM_ITEMS 5

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    explicit TxViewDelegate(const PlatformStyle *_platformStyle, QObject *parent=nullptr):
        QAbstractItemDelegate(parent), unit(RavenUnits::RVN),
        platformStyle(_platformStyle)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(TransactionTableModel::RawDecorationRole));
        QRect mainRect = option.rect;
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);
        icon = platformStyle->SingleColorIcon(icon);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);
        if(value.canConvert<QBrush>())
        {
            QBrush brush = qvariant_cast<QBrush>(value);
            foreground = brush.color();
        }

        painter->setPen(foreground);
        QRect boundingRect;
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, address, &boundingRect);

        if (index.data(TransactionTableModel::WatchonlyRole).toBool())
        {
            QIcon iconWatchonly = qvariant_cast<QIcon>(index.data(TransactionTableModel::WatchonlyDecorationRole));
            QRect watchonlyRect(boundingRect.right() + 5, mainRect.top()+ypad+halfheight, 16, halfheight);
            iconWatchonly.paint(painter, watchonlyRect);
        }

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
        QString amountText = index.data(TransactionTableModel::FormattedAmountRole).toString();
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(addressRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        QString assetName = index.data(TransactionTableModel::AssetNameRole).toString();
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, assetName);

        painter->setPen(option.palette.color(QPalette::Text));
        painter->drawText(amountRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;
    const PlatformStyle *platformStyle;

};
#include "overviewpage.moc"
#include "ravengui.h"

OverviewPage::OverviewPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage),
    clientModel(0),
    walletModel(0),
    currentBalance(-1),
    currentUnconfirmedBalance(-1),
    currentImmatureBalance(-1),
    currentWatchOnlyBalance(-1),
    currentWatchUnconfBalance(-1),
    currentWatchImmatureBalance(-1),
    txdelegate(new TxViewDelegate(platformStyle, this))
{
    ui->setupUi(this);

    // use a SingleColorIcon for the "out of sync warning" icon
    QIcon icon = platformStyle->SingleColorIcon(":/icons/warning");
    icon.addPixmap(icon.pixmap(QSize(64,64), QIcon::Normal), QIcon::Disabled); // also set the disabled icon because we are using a disabled QPushButton to work around missing HiDPI support of QLabel (https://bugreports.qt.io/browse/QTBUG-42503)
    ui->labelTransactionsStatus->setIcon(icon);
    ui->labelWalletStatus->setIcon(icon);
    ui->labelAssetStatus->setIcon(icon);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);
    connect(ui->labelWalletStatus, SIGNAL(clicked()), this, SLOT(handleOutOfSyncWarningClicks()));
    connect(ui->labelAssetStatus, SIGNAL(clicked()), this, SLOT(handleOutOfSyncWarningClicks()));
    connect(ui->labelTransactionsStatus, SIGNAL(clicked()), this, SLOT(handleOutOfSyncWarningClicks()));

    // Trigger the call to show the assets table if assets are active
    showAssets();
}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        Q_EMIT transactionClicked(filter->mapToSource(index));
}

void OverviewPage::handleOutOfSyncWarningClicks()
{
    Q_EMIT outOfSyncWarningClicked();
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance)
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentWatchOnlyBalance = watchOnlyBalance;
    currentWatchUnconfBalance = watchUnconfBalance;
    currentWatchImmatureBalance = watchImmatureBalance;
    ui->labelBalance->setText(RavenUnits::formatWithUnit(unit, balance, false, RavenUnits::separatorAlways));
    ui->labelUnconfirmed->setText(RavenUnits::formatWithUnit(unit, unconfirmedBalance, false, RavenUnits::separatorAlways));
    ui->labelImmature->setText(RavenUnits::formatWithUnit(unit, immatureBalance, false, RavenUnits::separatorAlways));
    ui->labelTotal->setText(RavenUnits::formatWithUnit(unit, balance + unconfirmedBalance + immatureBalance, false, RavenUnits::separatorAlways));
    ui->labelWatchAvailable->setText(RavenUnits::formatWithUnit(unit, watchOnlyBalance, false, RavenUnits::separatorAlways));
    ui->labelWatchPending->setText(RavenUnits::formatWithUnit(unit, watchUnconfBalance, false, RavenUnits::separatorAlways));
    ui->labelWatchImmature->setText(RavenUnits::formatWithUnit(unit, watchImmatureBalance, false, RavenUnits::separatorAlways));
    ui->labelWatchTotal->setText(RavenUnits::formatWithUnit(unit, watchOnlyBalance + watchUnconfBalance + watchImmatureBalance, false, RavenUnits::separatorAlways));

    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = immatureBalance != 0;
    bool showWatchOnlyImmature = watchImmatureBalance != 0;

    // for symmetry reasons also show immature label when the watch-only one is shown
    ui->labelImmature->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelImmatureText->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelWatchImmature->setVisible(showWatchOnlyImmature); // show watch-only immature balance
}

// show/hide watch-only labels
void OverviewPage::updateWatchOnlyLabels(bool showWatchOnly)
{
    ui->labelSpendable->setVisible(showWatchOnly);      // show spendable label (only when watch-only is active)
    ui->labelWatchonly->setVisible(showWatchOnly);      // show watch-only label
    ui->lineWatchBalance->setVisible(showWatchOnly);    // show watch-only balance separator line
    ui->labelWatchAvailable->setVisible(showWatchOnly); // show watch-only available balance
    ui->labelWatchPending->setVisible(showWatchOnly);   // show watch-only pending balance
    ui->labelWatchTotal->setVisible(showWatchOnly);     // show watch-only total balance

    if (!showWatchOnly)
        ui->labelWatchImmature->hide();
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

void OverviewPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model && model->getOptionsModel())
    {
        // Set up transaction list
        filter.reset(new TransactionFilterProxy());
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter.get());
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        assetFilter.reset(new QSortFilterProxyModel());
        assetFilter->setSourceModel(model->getAssetTableModel());
        ui->listAssets->setModel(assetFilter.get());

        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(),
                   model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance());
        connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(setBalance(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        updateWatchOnlyLabels(model->haveWatchOnly());
        connect(model, SIGNAL(notifyWatchonlyChanged(bool)), this, SLOT(updateWatchOnlyLabels(bool)));
    }

    // update the display unit, to not use the default ("RVN")
    updateDisplayUnit();
    ui->listAssets->resizeColumnsToContents();
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        if(currentBalance != -1)
            setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance,
                       currentWatchOnlyBalance, currentWatchUnconfBalance, currentWatchImmatureBalance);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = walletModel->getOptionsModel()->getDisplayUnit();

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
    if (AreAssetsDeployed()) {
        ui->labelAssetStatus->setVisible(fShow);
    }
}

void OverviewPage::showAssets()
{
    if (AreAssetsDeployed()) {
        ui->listAssets->show();
        ui->assetBalanceLabel->show();
        ui->labelAssetStatus->show();
    } else {
        ui->assetBalanceLabel->hide();
        ui->labelAssetStatus->hide();
        ui->listAssets->hide();
    }

    displayAssetInfo();
}

void OverviewPage::displayAssetInfo()
{
    ui->assetInfoTitleLabel->setText("<b>" + tr("Asset Activation Status") + "</b>");
    ui->assetInfoPercentageLabel->setText(tr("Current Percentage") + ":");
    ui->assetInfoStatusLabel->setText(tr("Status") + ":");
    ui->assetInfoBlockLabel->setText(tr("Target Percentage") + ":");
    ui->assetInfoPossibleLabel->setText(tr("Could Vote Pass") + ":");
    ui->assetInfoBlocksLeftLabel->setText(tr("Voting Block Cycle") + ":");

    const ThresholdState thresholdState = VersionBitsTipState(Params().GetConsensus(),
                                                              Consensus::DeploymentPos::DEPLOYMENT_ASSETS);
    auto startTime = Params().GetConsensus().vDeployments[Consensus::DeploymentPos::DEPLOYMENT_ASSETS].nStartTime * 1000;
    auto currentTime = GetTimeMillis();
    auto date = GUIUtil::dateTimeStr(startTime / 1000);

    QString status;
    switch (thresholdState) {
        case THRESHOLD_DEFINED:
            if (currentTime < startTime)
                status = tr("Waiting until ") + date;
            else {
                auto cycleWidth = Params().GetConsensus().nMinerConfirmationWindow;
                QString currentCount;
                currentCount.sprintf("%d of %d blocks", chainActive.Height() % cycleWidth, cycleWidth);
                status = tr("Waiting - ") +  currentCount;
            }
            break;
        case THRESHOLD_STARTED:
            status = tr("Voting Started");
            break;
        case THRESHOLD_LOCKED_IN:
            status = tr("Locked in - Not Active");
            break;
        case THRESHOLD_ACTIVE:
            status = tr("Active");
            break;
        case THRESHOLD_FAILED:
            status = tr("Failed");
            break;
    }

    if (thresholdState == THRESHOLD_ACTIVE) {
        hideAssetInfo();
        return;
    }

    ui->assetInfoStatusValue->setText(status);

    // Get the current height of the chain
    auto currentheight = chainActive.Height();

    auto heightLockedIn = VersionBitsTipStateSinceHeight(Params().GetConsensus(),
                                                         Consensus::DeploymentPos::DEPLOYMENT_ASSETS);
    auto cycleWidth = Params().GetConsensus().nMinerConfirmationWindow;
    auto difference = (currentheight - heightLockedIn + 1) % cycleWidth;
    QString currentCount;
    currentCount.sprintf("%d/%d blocks", difference, cycleWidth);

    if (thresholdState == THRESHOLD_STARTED) {
        BIP9Stats statsStruct = VersionBitsTipStatistics(Params().GetConsensus(),
                                                         Consensus::DeploymentPos::DEPLOYMENT_ASSETS);

        double targetDouble = double(statsStruct.threshold) / double(statsStruct.period);
        QString targetPercentage;
        targetPercentage.sprintf("%0.2f%%", targetDouble * 100);
        ui->assetInfoBlockValue->setText(targetPercentage);

        double currentDouble = double(statsStruct.count) / double(statsStruct.period);
        QString currentPercentage;
        currentPercentage.sprintf("%0.2f%%", currentDouble * 100);
        ui->assetInfoPercentageValue->setText(currentPercentage);

        QString possible = statsStruct.possible ? tr("yes") : tr("no");
        ui->assetInfoPossibleValue->setText(possible);

        ui->assetInfoBlocksLeftValue->setText(currentCount);

        ui->assetInfoPercentageValue->show();
        ui->assetInfoBlockValue->show();
        ui->assetInfoPercentageLabel->show();
        ui->assetInfoBlockLabel->show();
        ui->assetInfoPossibleLabel->show();
        ui->assetInfoPossibleValue->show();
        ui->assetInfoBlocksLeftLabel->show();
        ui->assetInfoBlocksLeftValue->show();
    } else if (thresholdState == THRESHOLD_LOCKED_IN) {

        ui->assetInfoBlockLabel->setText(tr("Active in") + ":");
        ui->assetInfoBlockValue->setText(currentCount);

        ui->assetInfoPercentageValue->hide();
        ui->assetInfoPercentageLabel->hide();
        ui->assetInfoPossibleLabel->hide();
        ui->assetInfoPossibleValue->hide();
        ui->assetInfoBlocksLeftLabel->hide();
        ui->assetInfoBlocksLeftValue->hide();
    } else {
        ui->assetInfoPercentageValue->hide();
        ui->assetInfoBlockValue->hide();
        ui->assetInfoPercentageLabel->hide();
        ui->assetInfoBlockLabel->hide();
        ui->assetInfoPossibleLabel->hide();
        ui->assetInfoPossibleValue->hide();
        ui->assetInfoBlocksLeftLabel->hide();
        ui->assetInfoBlocksLeftValue->hide();
    }
}

void OverviewPage::hideAssetInfo()
{
    ui->assetInfoPercentageValue->hide();
    ui->assetInfoBlockValue->hide();
    ui->assetInfoStatusValue->hide();
    ui->assetInfoPossibleValue->hide();
    ui->assetInfoBlocksLeftValue->hide();

    ui->assetInfoTitleLabel->hide();
    ui->assetInfoBlockLabel->hide();
    ui->assetInfoStatusLabel->hide();
    ui->assetInfoPercentageLabel->hide();
    ui->assetInfoPossibleLabel->hide();
    ui->assetInfoBlocksLeftLabel->hide();
}
