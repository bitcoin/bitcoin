// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2014-2020 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/overviewpage.h>
#include <qt/forms/ui_overviewpage.h>

#include <qt/bitcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/guiutil.h>
#include <init.h>
#include <qt/optionsmodel.h>
#include <qt/transactionfilterproxy.h>
#include <qt/transactiontablemodel.h>
#include <qt/utilitydialog.h>
#include <qt/walletmodel.h>

#include <masternode/masternode-sync.h>
#include <privatesend/privatesend-client.h>

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QSettings>
#include <QTimer>

#define ITEM_HEIGHT 54
#define NUM_ITEMS_DISABLED 5
#define NUM_ITEMS_ENABLED_NORMAL 6
#define NUM_ITEMS_ENABLED_ADVANCED 8

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    explicit TxViewDelegate(QObject* parent = nullptr) :
        QAbstractItemDelegate(), unit(BitcoinUnits::DASH)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QRect mainRect = option.rect;
        int xspace = 8;
        int ypad = 8;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect rectTopHalf(mainRect.left() + xspace, mainRect.top() + ypad, mainRect.width() - xspace, halfheight);
        QRect rectBottomHalf(mainRect.left() + xspace, mainRect.top() + ypad + halfheight + 5, mainRect.width() - xspace, halfheight);
        QRect rectBounding;
        QColor colorForeground;
        qreal initialFontSize = painter->font().pointSizeF();

        // Grab model indexes for desired data from TransactionTableModel
        QModelIndex indexDate = index.sibling(index.row(), TransactionTableModel::Date);
        QModelIndex indexAmount = index.sibling(index.row(), TransactionTableModel::Amount);
        QModelIndex indexAddress = index.sibling(index.row(), TransactionTableModel::ToAddress);

        // Draw first line (with slightly bigger font than the second line will get)
        // Content: Date/Time, Optional IS indicator, Amount
        painter->setFont(GUIUtil::getFont(GUIUtil::FontWeight::Normal, false, GUIUtil::getScaledFontSize(initialFontSize * 1.17)));
        // Date/Time
        colorForeground = qvariant_cast<QColor>(indexDate.data(Qt::ForegroundRole));
        QString strDate = indexDate.data(Qt::DisplayRole).toString();
        painter->setPen(colorForeground);
        painter->drawText(rectTopHalf, Qt::AlignLeft | Qt::AlignVCenter, strDate, &rectBounding);
        // Optional IS indicator
        QIcon iconInstantSend = qvariant_cast<QIcon>(indexAddress.data(TransactionTableModel::RawDecorationRole));
        QRect rectInstantSend(rectBounding.right() + 5, rectTopHalf.top(), 16, halfheight);
        iconInstantSend.paint(painter, rectInstantSend);
        // Amount
        colorForeground = qvariant_cast<QColor>(indexAmount.data(Qt::ForegroundRole));
        // Note: do NOT use Qt::DisplayRole, have format properly here
        qint64 nAmount = index.data(TransactionTableModel::AmountRole).toLongLong();
        QString strAmount = BitcoinUnits::floorWithUnit(unit, nAmount, true, BitcoinUnits::separatorAlways);
        painter->setPen(colorForeground);
        painter->drawText(rectTopHalf, Qt::AlignRight | Qt::AlignVCenter, strAmount);

        // Draw second line (with the initial font)
        // Content: Address/label, Optional Watchonly indicator
        painter->setFont(GUIUtil::getFont(GUIUtil::FontWeight::Normal, false, GUIUtil::getScaledFontSize(initialFontSize)));
        // Address/Label
        colorForeground = qvariant_cast<QColor>(indexAddress.data(Qt::ForegroundRole));
        QString address = indexAddress.data(Qt::DisplayRole).toString();
        painter->setPen(colorForeground);
        painter->drawText(rectBottomHalf, Qt::AlignLeft | Qt::AlignVCenter, address, &rectBounding);
        // Optional Watchonly indicator
        if (index.data(TransactionTableModel::WatchonlyRole).toBool())
        {
            QIcon iconWatchonly = qvariant_cast<QIcon>(index.data(TransactionTableModel::WatchonlyDecorationRole));
            QRect rectWatchonly(rectBounding.right() + 5, rectBottomHalf.top(), 16, halfheight);
            iconWatchonly.paint(painter, rectWatchonly);
        }

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(ITEM_HEIGHT, ITEM_HEIGHT);
    }

    int unit;

};
#include <qt/overviewpage.moc>

OverviewPage::OverviewPage(QWidget* parent) :
    QWidget(parent),
    timer(nullptr),
    ui(new Ui::OverviewPage),
    clientModel(0),
    walletModel(0),
    currentBalance(-1),
    currentUnconfirmedBalance(-1),
    currentImmatureBalance(-1),
    currentWatchOnlyBalance(-1),
    currentWatchUnconfBalance(-1),
    currentWatchImmatureBalance(-1),
    cachedNumISLocks(-1),
    txdelegate(new TxViewDelegate(this))
{
    ui->setupUi(this);

    GUIUtil::setFont({ui->label_4,
                      ui->label_5,
                      ui->labelPrivateSendHeader
                     }, GUIUtil::FontWeight::Bold, 16);

    GUIUtil::setFont({ui->labelTotalText,
                      ui->labelWatchTotal,
                      ui->labelTotal
                     }, GUIUtil::FontWeight::Bold, 14);

    GUIUtil::setFont({ui->labelBalanceText,
                      ui->labelPendingText,
                      ui->labelImmatureText,
                      ui->labelWatchonly,
                      ui->labelSpendable
                     }, GUIUtil::FontWeight::Bold);

    GUIUtil::updateFonts();

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    // Note: minimum height of listTransactions will be set later in updateAdvancedPSUI() to reflect actual settings
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));

    // init "out of sync" warning labels
    ui->labelWalletStatus->setText("(" + tr("out of sync") + ")");
    ui->labelPrivateSendSyncStatus->setText("(" + tr("out of sync") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("out of sync") + ")");

    // hide PS frame (helps to preserve saved size)
    // we'll setup and make it visible in privateSendStatus() later
    ui->framePrivateSend->setVisible(false);

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);

    // Disable privateSendClient builtin support for automatic backups while we are in GUI,
    // we'll handle automatic backups and user warnings in privateSendStatus()
    for (auto& pair : privateSendClientManagers) {
        pair.second->fCreateAutoBackups = false;
    }

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(privateSendStatus()));
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
    if(timer) disconnect(timer, SIGNAL(timeout()), this, SLOT(privateSendStatus()));
    delete ui;
}

void OverviewPage::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, const CAmount& anonymizedBalance, const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance)
{
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentAnonymizedBalance = anonymizedBalance;
    currentWatchOnlyBalance = watchOnlyBalance;
    currentWatchUnconfBalance = watchUnconfBalance;
    currentWatchImmatureBalance = watchImmatureBalance;
    ui->labelBalance->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, balance, false, BitcoinUnits::separatorAlways));
    ui->labelUnconfirmed->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, unconfirmedBalance, false, BitcoinUnits::separatorAlways));
    ui->labelImmature->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, immatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelAnonymized->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, anonymizedBalance, false, BitcoinUnits::separatorAlways));
    ui->labelTotal->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, balance + unconfirmedBalance + immatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchAvailable->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, watchOnlyBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchPending->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, watchUnconfBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchImmature->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, watchImmatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchTotal->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, watchOnlyBalance + watchUnconfBalance + watchImmatureBalance, false, BitcoinUnits::separatorAlways));

    // only show immature (newly mined) balance if it's non-zero or in UI debug mode, so as not to complicate things
    // for the non-mining users
    bool fDebugUI = gArgs.GetBoolArg("-debug-ui", false);
    bool showImmature = fDebugUI || immatureBalance != 0;
    bool showWatchOnlyImmature = fDebugUI || watchImmatureBalance != 0;

    // for symmetry reasons also show immature label when the watch-only one is shown
    ui->labelImmature->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelImmatureText->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelWatchImmature->setVisible(showWatchOnlyImmature); // show watch-only immature balance

    updatePrivateSendProgress();

    if (walletModel) {
        int numISLocks = walletModel->getNumISLocks();
        if(cachedNumISLocks != numISLocks) {
            cachedNumISLocks = numISLocks;
            ui->listTransactions->update();
        }
    }
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

    if (!showWatchOnly){
        ui->labelWatchImmature->hide();
    }
    else{
        ui->labelBalance->setIndent(20);
        ui->labelUnconfirmed->setIndent(20);
        ui->labelImmature->setIndent(20);
        ui->labelTotal->setIndent(20);
    }
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
        // update the display unit, to not use the default ("DASH")
        updateDisplayUnit();
        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(), model->getAnonymizedBalance(),
                   model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance());
        connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(setBalance(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        updateWatchOnlyLabels(model->haveWatchOnly() || gArgs.GetBoolArg("-debug-ui", false));
        connect(model, SIGNAL(notifyWatchonlyChanged(bool)), this, SLOT(updateWatchOnlyLabels(bool)));

        // explicitly update PS frame and transaction list to reflect actual settings
        updateAdvancedPSUI(model->getOptionsModel()->getShowAdvancedPSUI());

        connect(model->getOptionsModel(), SIGNAL(privateSendRoundsChanged()), this, SLOT(updatePrivateSendProgress()));
        connect(model->getOptionsModel(), SIGNAL(privateSentAmountChanged()), this, SLOT(updatePrivateSendProgress()));
        connect(model->getOptionsModel(), SIGNAL(advancedPSUIChanged(bool)), this, SLOT(updateAdvancedPSUI(bool)));
        connect(model->getOptionsModel(), &OptionsModel::privateSendEnabledChanged, [=]() {
            privateSendStatus(true);
        });

        connect(ui->togglePrivateSend, SIGNAL(clicked()), this, SLOT(togglePrivateSend()));

        // privatesend buttons will not react to spacebar must be clicked on
        ui->togglePrivateSend->setFocusPolicy(Qt::NoFocus);
    }
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        nDisplayUnit = walletModel->getOptionsModel()->getDisplayUnit();
        if(currentBalance != -1)
            setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance, currentAnonymizedBalance,
                       currentWatchOnlyBalance, currentWatchUnconfBalance, currentWatchImmatureBalance);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = nDisplayUnit;

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
    ui->labelPrivateSendSyncStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}

void OverviewPage::updatePrivateSendProgress()
{
    if(!masternodeSync.IsBlockchainSynced() || ShutdownRequested()) return;

    if(!walletModel) return;

    QString strAmountAndRounds;
    QString strPrivateSendAmount = BitcoinUnits::formatHtmlWithUnit(nDisplayUnit, CPrivateSendClientOptions::GetAmount() * COIN, false, BitcoinUnits::separatorAlways);

    if(currentBalance == 0)
    {
        ui->privateSendProgress->setValue(0);
        ui->privateSendProgress->setToolTip(tr("No inputs detected"));

        // when balance is zero just show info from settings
        strPrivateSendAmount = strPrivateSendAmount.remove(strPrivateSendAmount.indexOf("."), BitcoinUnits::decimals(nDisplayUnit) + 1);
        strAmountAndRounds = strPrivateSendAmount + " / " + tr("%n Rounds", "", CPrivateSendClientOptions::GetRounds());

        ui->labelAmountRounds->setToolTip(tr("No inputs detected"));
        ui->labelAmountRounds->setText(strAmountAndRounds);
        return;
    }

    CAmount nAnonymizableBalance = walletModel->getAnonymizableBalance(false, false);

    CAmount nMaxToAnonymize = nAnonymizableBalance + currentAnonymizedBalance;

    // If it's more than the anon threshold, limit to that.
    if (nMaxToAnonymize > CPrivateSendClientOptions::GetAmount() * COIN) nMaxToAnonymize = CPrivateSendClientOptions::GetAmount() * COIN;

    if(nMaxToAnonymize == 0) return;

    if (nMaxToAnonymize >= CPrivateSendClientOptions::GetAmount() * COIN) {
        ui->labelAmountRounds->setToolTip(tr("Found enough compatible inputs to mix %1")
                                          .arg(strPrivateSendAmount));
        strPrivateSendAmount = strPrivateSendAmount.remove(strPrivateSendAmount.indexOf("."), BitcoinUnits::decimals(nDisplayUnit) + 1);
        strAmountAndRounds = strPrivateSendAmount + " / " + tr("%n Rounds", "", CPrivateSendClientOptions::GetRounds());
    } else {
        QString strMaxToAnonymize = BitcoinUnits::formatHtmlWithUnit(nDisplayUnit, nMaxToAnonymize, false, BitcoinUnits::separatorAlways);
        ui->labelAmountRounds->setToolTip(tr("Not enough compatible inputs to mix <span style='%1'>%2</span>,<br>"
                                             "will mix <span style='%1'>%3</span> instead")
                                          .arg(GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_ERROR))
                                          .arg(strPrivateSendAmount)
                                          .arg(strMaxToAnonymize));
        strMaxToAnonymize = strMaxToAnonymize.remove(strMaxToAnonymize.indexOf("."), BitcoinUnits::decimals(nDisplayUnit) + 1);
        strAmountAndRounds = "<span style='" + GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_ERROR) + "'>" +
                QString(BitcoinUnits::factor(nDisplayUnit) == 1 ? "" : "~") + strMaxToAnonymize +
                " / " + tr("%n Rounds", "", CPrivateSendClientOptions::GetRounds()) + "</span>";
    }
    ui->labelAmountRounds->setText(strAmountAndRounds);

    if (!fShowAdvancedPSUI) return;

    CAmount nDenominatedConfirmedBalance;
    CAmount nDenominatedUnconfirmedBalance;
    CAmount nNormalizedAnonymizedBalance;
    float nAverageAnonymizedRounds;

    nDenominatedConfirmedBalance = walletModel->getDenominatedBalance(false);
    nDenominatedUnconfirmedBalance = walletModel->getDenominatedBalance(true);
    nNormalizedAnonymizedBalance = walletModel->getNormalizedAnonymizedBalance();
    nAverageAnonymizedRounds = walletModel->getAverageAnonymizedRounds();

    // calculate parts of the progress, each of them shouldn't be higher than 1
    // progress of denominating
    float denomPart = 0;
    // mixing progress of denominated balance
    float anonNormPart = 0;
    // completeness of full amount anonymization
    float anonFullPart = 0;

    CAmount denominatedBalance = nDenominatedConfirmedBalance + nDenominatedUnconfirmedBalance;
    denomPart = (float)denominatedBalance / nMaxToAnonymize;
    denomPart = denomPart > 1 ? 1 : denomPart;
    denomPart *= 100;

    anonNormPart = (float)nNormalizedAnonymizedBalance / nMaxToAnonymize;
    anonNormPart = anonNormPart > 1 ? 1 : anonNormPart;
    anonNormPart *= 100;

    anonFullPart = (float)currentAnonymizedBalance / nMaxToAnonymize;
    anonFullPart = anonFullPart > 1 ? 1 : anonFullPart;
    anonFullPart *= 100;

    // apply some weights to them ...
    float denomWeight = 1;
    float anonNormWeight = CPrivateSendClientOptions::GetRounds();
    float anonFullWeight = 2;
    float fullWeight = denomWeight + anonNormWeight + anonFullWeight;
    // ... and calculate the whole progress
    float denomPartCalc = ceilf((denomPart * denomWeight / fullWeight) * 100) / 100;
    float anonNormPartCalc = ceilf((anonNormPart * anonNormWeight / fullWeight) * 100) / 100;
    float anonFullPartCalc = ceilf((anonFullPart * anonFullWeight / fullWeight) * 100) / 100;
    float progress = denomPartCalc + anonNormPartCalc + anonFullPartCalc;
    if(progress >= 100) progress = 100;

    ui->privateSendProgress->setValue(progress);

    QString strToolPip = ("<b>" + tr("Overall progress") + ": %1%</b><br/>" +
                          tr("Denominated") + ": %2%<br/>" +
                          tr("Partially mixed") + ": %3%<br/>" +
                          tr("Mixed") + ": %4%<br/>" +
                          tr("Denominated inputs have %5 of %n rounds on average", "", CPrivateSendClientOptions::GetRounds()))
            .arg(progress).arg(denomPart).arg(anonNormPart).arg(anonFullPart)
            .arg(nAverageAnonymizedRounds);
    ui->privateSendProgress->setToolTip(strToolPip);
}

void OverviewPage::updateAdvancedPSUI(bool fShowAdvancedPSUI) {
    this->fShowAdvancedPSUI = fShowAdvancedPSUI;
    privateSendStatus(true);
}

void OverviewPage::privateSendStatus(bool fForce)
{
    if (!fForce && (!masternodeSync.IsBlockchainSynced() || ShutdownRequested())) return;

    if(!walletModel) return;

    // Disable any PS UI for masternode or when autobackup is disabled or failed for whatever reason
    if (fMasternodeMode || nWalletBackups <= 0) {
        DisablePrivateSendCompletely();
        if (nWalletBackups <= 0) {
            ui->labelPrivateSendEnabled->setToolTip(tr("Automatic backups are disabled, no mixing available!"));
        }
        return;
    }

    bool fIsEnabled = CPrivateSendClientOptions::IsEnabled();
    ui->framePrivateSend->setVisible(fIsEnabled);
    if (!fIsEnabled) {
        SetupTransactionList(NUM_ITEMS_DISABLED);
        if (timer != nullptr) {
            timer->stop();
        }
        return;
    }

    if (timer != nullptr && !timer->isActive()) {
        timer->start(1000);
    }

    // Wrap all privatesend related widgets we want to show/hide state based.
    // Value of the map contains a flag if this widget belongs to the advanced
    // PrivateSend UI option or not. True if it does, false if not.
    std::map<QWidget*, bool> privateSendWidgets = {
        {ui->labelCompletitionText, true},
        {ui->privateSendProgress, true},
        {ui->labelSubmittedDenomText, true},
        {ui->labelSubmittedDenom, true},
        {ui->labelAmountAndRoundsText, false},
        {ui->labelAmountRounds, false}
    };

    auto setWidgetsVisible = [&](bool fVisible) {
        static bool fInitial{true};
        static bool fLastVisible{false};
        static bool fLastShowAdvanced{false};
        // Only update the widget's visibility if something changed since the last call of setWidgetsVisible
        if (fLastShowAdvanced == fShowAdvancedPSUI && fLastVisible == fVisible) {
            if (fInitial) {
                fInitial = false;
            } else {
                return;
            }
        }
        // Set visible if: fVisible and not advanced UI element or advanced ui element and advanced ui active
        for (const auto& it : privateSendWidgets) {
            it.first->setVisible(fVisible && (!it.second || it.second == fShowAdvancedPSUI));
        }
        fLastVisible = fVisible;
        fLastShowAdvanced = fShowAdvancedPSUI;
    };

    static int64_t nLastDSProgressBlockTime = 0;
    int nBestHeight = clientModel->getNumBlocks();

    auto it = privateSendClientManagers.find(walletModel->getWallet()->GetName());
    if (it == privateSendClientManagers.end()) {
        // nothing to do
        return;
    }
    CPrivateSendClientManager* privateSendClientManager = it->second;

    // We are processing more than 1 block per second, we'll just leave
    if(nBestHeight > privateSendClientManager->nCachedNumBlocks && GetTime() - nLastDSProgressBlockTime <= 1) return;
    nLastDSProgressBlockTime = GetTime();

    QString strKeysLeftText(tr("keys left: %1").arg(walletModel->getKeysLeftSinceAutoBackup()));
    if(walletModel->getKeysLeftSinceAutoBackup() < PRIVATESEND_KEYS_THRESHOLD_WARNING) {
        strKeysLeftText = "<span style='" + GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_ERROR) + "'>" + strKeysLeftText + "</span>";
    }
    ui->labelPrivateSendEnabled->setToolTip(strKeysLeftText);

    if (!privateSendClientManager->IsMixing()) {
        if (nBestHeight != privateSendClientManager->nCachedNumBlocks) {
            privateSendClientManager->nCachedNumBlocks = nBestHeight;
            updatePrivateSendProgress();
        }

        setWidgetsVisible(false);
        ui->togglePrivateSend->setText(tr("Start Mixing"));

        QString strEnabled = tr("Disabled");
        // Show how many keys left in advanced PS UI mode only
        if (fShowAdvancedPSUI) strEnabled += ", " + strKeysLeftText;
        ui->labelPrivateSendEnabled->setText(strEnabled);

        // If mixing isn't active always show the lower number of txes because there are
        // anyway the most PS widgets hidden.
        SetupTransactionList(NUM_ITEMS_ENABLED_NORMAL);

        return;
    } else {
        SetupTransactionList(fShowAdvancedPSUI ? NUM_ITEMS_ENABLED_ADVANCED : NUM_ITEMS_ENABLED_NORMAL);
    }

    // Warn user that wallet is running out of keys
    // NOTE: we do NOT warn user and do NOT create autobackups if mixing is not running
    if (nWalletBackups > 0 && walletModel->getKeysLeftSinceAutoBackup() < PRIVATESEND_KEYS_THRESHOLD_WARNING) {
        QSettings settings;
        if(settings.value("fLowKeysWarning").toBool()) {
            QString strWarn =   tr("Very low number of keys left since last automatic backup!") + "<br><br>" +
                                tr("We are about to create a new automatic backup for you, however "
                                   "<span style='%1'> you should always make sure you have backups "
                                   "saved in some safe place</span>!").arg(GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_COMMAND)) + "<br><br>" +
                                tr("Note: You can turn this message off in options.");
            ui->labelPrivateSendEnabled->setToolTip(strWarn);
            LogPrint(BCLog::PRIVATESEND, "OverviewPage::privateSendStatus -- Very low number of keys left since last automatic backup, warning user and trying to create new backup...\n");
            QMessageBox::warning(this, "PrivateSend", strWarn, QMessageBox::Ok, QMessageBox::Ok);
        } else {
            LogPrint(BCLog::PRIVATESEND, "OverviewPage::privateSendStatus -- Very low number of keys left since last automatic backup, skipping warning and trying to create new backup...\n");
        }

        QString strBackupWarning;
        QString strBackupError;
        if(!walletModel->autoBackupWallet(strBackupWarning, strBackupError)) {
            if (!strBackupWarning.isEmpty()) {
                // It's still more or less safe to continue but warn user anyway
                LogPrint(BCLog::PRIVATESEND, "OverviewPage::privateSendStatus -- WARNING! Something went wrong on automatic backup: %s\n", strBackupWarning.toStdString());

                QMessageBox::warning(this, "PrivateSend",
                    tr("WARNING! Something went wrong on automatic backup") + ":<br><br>" + strBackupWarning,
                    QMessageBox::Ok, QMessageBox::Ok);
            }
            if (!strBackupError.isEmpty()) {
                // Things are really broken, warn user and stop mixing immediately
                LogPrint(BCLog::PRIVATESEND, "OverviewPage::privateSendStatus -- ERROR! Failed to create automatic backup: %s\n", strBackupError.toStdString());

                QMessageBox::warning(this, "PrivateSend",
                    tr("ERROR! Failed to create automatic backup") + ":<br><br>" + strBackupError + "<br>" +
                    tr("Mixing is disabled, please close your wallet and fix the issue!"),
                    QMessageBox::Ok, QMessageBox::Ok);
            }
        }
    }

    QString strEnabled = privateSendClientManager->IsMixing() ? tr("Enabled") : tr("Disabled");
    // Show how many keys left in advanced PS UI mode only
    if(fShowAdvancedPSUI) strEnabled += ", " + strKeysLeftText;
    ui->labelPrivateSendEnabled->setText(strEnabled);

    if(nWalletBackups == -1) {
        // Automatic backup failed, nothing else we can do until user fixes the issue manually
        DisablePrivateSendCompletely();

        QString strError =  tr("ERROR! Failed to create automatic backup") + ", " +
                            tr("see debug.log for details.") + "<br><br>" +
                            tr("Mixing is disabled, please close your wallet and fix the issue!");
        ui->labelPrivateSendEnabled->setToolTip(strError);

        return;
    } else if(nWalletBackups == -2) {
        // We were able to create automatic backup but keypool was not replenished because wallet is locked.
        QString strWarning = tr("WARNING! Failed to replenish keypool, please unlock your wallet to do so.");
        ui->labelPrivateSendEnabled->setToolTip(strWarning);
    }

    // check privatesend status and unlock if needed
    if(nBestHeight != privateSendClientManager->nCachedNumBlocks) {
        // Balance and number of transactions might have changed
        privateSendClientManager->nCachedNumBlocks = nBestHeight;
        updatePrivateSendProgress();
    }

    setWidgetsVisible(true);

    ui->labelSubmittedDenom->setText(QString(privateSendClientManager->GetSessionDenoms().c_str()));
}

void OverviewPage::togglePrivateSend(){
    QSettings settings;
    // Popup some information on first mixing
    QString hasMixed = settings.value("hasMixed").toString();
    if(hasMixed.isEmpty()){
        QMessageBox::information(this, "PrivateSend",
                tr("If you don't want to see internal PrivateSend fees/transactions select \"Most Common\" as Type on the \"Transactions\" tab."),
                QMessageBox::Ok, QMessageBox::Ok);
        settings.setValue("hasMixed", "hasMixed");
    }

    auto it = privateSendClientManagers.find(walletModel->getWallet()->GetName());
    if (it == privateSendClientManagers.end()) {
        // nothing to do
        return;
    }
    CPrivateSendClientManager* privateSendClientManager = it->second;

    if (!privateSendClientManager->IsMixing()) {
        const CAmount nMinAmount = CPrivateSend::GetSmallestDenomination() + CPrivateSend::GetMaxCollateralAmount();
        if(currentBalance < nMinAmount){
            QString strMinAmount(BitcoinUnits::formatWithUnit(nDisplayUnit, nMinAmount));
            QMessageBox::warning(this, "PrivateSend",
                tr("PrivateSend requires at least %1 to use.").arg(strMinAmount),
                QMessageBox::Ok, QMessageBox::Ok);
            return;
        }

        // if wallet is locked, ask for a passphrase
        if (walletModel && walletModel->getEncryptionStatus() == WalletModel::Locked)
        {
            WalletModel::UnlockContext ctx(walletModel->requestUnlock(true));
            if(!ctx.isValid())
            {
                //unlock was cancelled
                privateSendClientManager->nCachedNumBlocks = std::numeric_limits<int>::max();
                QMessageBox::warning(this, "PrivateSend",
                    tr("Wallet is locked and user declined to unlock. Disabling PrivateSend."),
                    QMessageBox::Ok, QMessageBox::Ok);
                LogPrint(BCLog::PRIVATESEND, "OverviewPage::togglePrivateSend -- Wallet is locked and user declined to unlock. Disabling PrivateSend.\n");
                return;
            }
        }

    }

    privateSendClientManager->nCachedNumBlocks = std::numeric_limits<int>::max();

    if (privateSendClientManager->IsMixing()) {
        ui->togglePrivateSend->setText(tr("Start Mixing"));
        privateSendClientManager->ResetPool();
        privateSendClientManager->StopMixing();
    } else {
        ui->togglePrivateSend->setText(tr("Stop Mixing"));
        privateSendClientManager->StartMixing(walletModel->getWallet());
    }
}

void OverviewPage::SetupTransactionList(int nNumItems)
{
    if (walletModel == nullptr || walletModel->getTransactionTableModel() == nullptr) {
        return;
    }

    // Set up transaction list
    if (filter == nullptr) {
        filter.reset(new TransactionFilterProxy());
        filter->setSourceModel(walletModel->getTransactionTableModel());
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);
        ui->listTransactions->setModel(filter.get());
    }

    if (filter->rowCount() == nNumItems) {
        return;
    }

    filter->setLimit(nNumItems);
    ui->listTransactions->setMinimumHeight(nNumItems * ITEM_HEIGHT);
}

void OverviewPage::DisablePrivateSendCompletely() {
    ui->togglePrivateSend->setText("(" + tr("Disabled") + ")");
    ui->framePrivateSend->setEnabled(false);
    if (nWalletBackups <= 0) {
        ui->labelPrivateSendEnabled->setText("<span style='" + GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_ERROR) + "'>(" + tr("Disabled") + ")</span>");
    }
    auto it = privateSendClientManagers.find(walletModel->getWallet()->GetName());
    if (it == privateSendClientManagers.end()) {
        // nothing to do
        return;
    }
    it->second->StopMixing();
}
