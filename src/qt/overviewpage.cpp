// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2014-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/overviewpage.h>
#include <qt/forms/ui_overviewpage.h>

#include <qt/bitcoinunits.h>
#include <qt/clientmodel.h>
#include <qt/guiutil.h>
#include <qt/optionsmodel.h>
#include <qt/transactionfilterproxy.h>
#include <qt/transactiontablemodel.h>
#include <qt/utilitydialog.h>
#include <qt/walletmodel.h>

#include <coinjoin/coinjoin-client-options.h>

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QSettings>
#include <QTimer>

#define ITEM_HEIGHT 54
#define NUM_ITEMS_DISABLED 5
#define NUM_ITEMS_ENABLED_NORMAL 6
#define NUM_ITEMS_ENABLED_ADVANCED 8

Q_DECLARE_METATYPE(interfaces::WalletBalances)

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    explicit TxViewDelegate(QObject* parent = nullptr) :
        QAbstractItemDelegate(), unit(BitcoinUnits::DASH)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const override
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

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
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
    cachedNumISLocks(-1),
    txdelegate(new TxViewDelegate(this))
{
    ui->setupUi(this);

    GUIUtil::setFont({ui->label_4,
                      ui->label_5,
                      ui->labelCoinJoinHeader
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

    m_balances.balance = -1;

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    // Note: minimum height of listTransactions will be set later in updateAdvancedCJUI() to reflect actual settings
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));

    // init "out of sync" warning labels
    ui->labelWalletStatus->setText("(" + tr("out of sync") + ")");
    ui->labelCoinJoinSyncStatus->setText("(" + tr("out of sync") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("out of sync") + ")");

    ui->labelAnonymizedText->setText(tr("%1 Balance").arg("CoinJoin"));

    // hide PS frame (helps to preserve saved size)
    // we'll setup and make it visible in coinJoinStatus() later
    ui->frameCoinJoin->setVisible(false);

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(coinJoinStatus()));
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
    if(timer) disconnect(timer, SIGNAL(timeout()), this, SLOT(coinJoinStatus()));
    delete ui;
}

void OverviewPage::setBalance(const interfaces::WalletBalances& balances)
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    m_balances = balances;
    ui->labelBalance->setText(BitcoinUnits::floorHtmlWithUnit(unit, balances.balance, false, BitcoinUnits::separatorAlways));
    ui->labelUnconfirmed->setText(BitcoinUnits::floorHtmlWithUnit(unit, balances.unconfirmed_balance, false, BitcoinUnits::separatorAlways));
    ui->labelImmature->setText(BitcoinUnits::floorHtmlWithUnit(unit, balances.immature_balance, false, BitcoinUnits::separatorAlways));
    ui->labelAnonymized->setText(BitcoinUnits::floorHtmlWithUnit(unit, balances.anonymized_balance, false, BitcoinUnits::separatorAlways));
    ui->labelTotal->setText(BitcoinUnits::floorHtmlWithUnit(unit, balances.balance + balances.unconfirmed_balance + balances.immature_balance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchAvailable->setText(BitcoinUnits::floorHtmlWithUnit(unit, balances.watch_only_balance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchPending->setText(BitcoinUnits::floorHtmlWithUnit(unit, balances.unconfirmed_watch_only_balance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchImmature->setText(BitcoinUnits::floorHtmlWithUnit(unit, balances.immature_watch_only_balance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchTotal->setText(BitcoinUnits::floorHtmlWithUnit(unit, balances.watch_only_balance + balances.unconfirmed_watch_only_balance + balances.immature_watch_only_balance, false, BitcoinUnits::separatorAlways));

    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool fDebugUI = gArgs.GetBoolArg("-debug-ui", false);
    bool showImmature = fDebugUI || balances.immature_balance != 0;
    bool showWatchOnlyImmature = fDebugUI || balances.immature_watch_only_balance != 0;

    // for symmetry reasons also show immature label when the watch-only one is shown
    ui->labelImmature->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelImmatureText->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelWatchImmature->setVisible(showWatchOnlyImmature); // show watch-only immature balance

    updateCoinJoinProgress();

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
        interfaces::Wallet& wallet = model->wallet();
        interfaces::WalletBalances balances = wallet.getBalances();
        setBalance(balances);
        connect(model, SIGNAL(balanceChanged(interfaces::WalletBalances)), this, SLOT(setBalance(interfaces::WalletBalances)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        updateWatchOnlyLabels(wallet.haveWatchOnly() || gArgs.GetBoolArg("-debug-ui", false));
        connect(model, SIGNAL(notifyWatchonlyChanged(bool)), this, SLOT(updateWatchOnlyLabels(bool)));

        // explicitly update PS frame and transaction list to reflect actual settings
        updateAdvancedCJUI(model->getOptionsModel()->getShowAdvancedCJUI());

        connect(model->getOptionsModel(), SIGNAL(coinJoinRoundsChanged()), this, SLOT(updateCoinJoinProgress()));
        connect(model->getOptionsModel(), SIGNAL(coinJoinAmountChanged()), this, SLOT(updateCoinJoinProgress()));
        connect(model->getOptionsModel(), SIGNAL(AdvancedCJUIChanged(bool)), this, SLOT(updateAdvancedCJUI(bool)));
        connect(model->getOptionsModel(), &OptionsModel::coinJoinEnabledChanged, [=]() {
            coinJoinStatus(true);
        });

        // Disable coinJoinClient builtin support for automatic backups while we are in GUI,
        // we'll handle automatic backups and user warnings in coinJoinStatus()
        walletModel->coinJoin().disableAutobackups();

        connect(ui->toggleCoinJoin, SIGNAL(clicked()), this, SLOT(toggleCoinJoin()));

        // coinjoin buttons will not react to spacebar must be clicked on
        ui->toggleCoinJoin->setFocusPolicy(Qt::NoFocus);
    }
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        nDisplayUnit = walletModel->getOptionsModel()->getDisplayUnit();
        if (m_balances.balance != -1) {
            setBalance(m_balances);
        }

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
    ui->labelCoinJoinSyncStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}

void OverviewPage::updateCoinJoinProgress()
{
    if (!walletModel || !clientModel || clientModel->node().shutdownRequested() || !clientModel->masternodeSync().isBlockchainSynced()) return;

    QString strAmountAndRounds;
    QString strCoinJoinAmount = BitcoinUnits::formatHtmlWithUnit(nDisplayUnit, clientModel->coinJoinOptions().getAmount() * COIN, false, BitcoinUnits::separatorAlways);

    if(m_balances.balance == 0)
    {
        ui->coinJoinProgress->setValue(0);
        ui->coinJoinProgress->setToolTip(tr("No inputs detected"));

        // when balance is zero just show info from settings
        strCoinJoinAmount = strCoinJoinAmount.remove(strCoinJoinAmount.indexOf("."), BitcoinUnits::decimals(nDisplayUnit) + 1);
        strAmountAndRounds = strCoinJoinAmount + " / " + tr("%n Rounds", "", clientModel->coinJoinOptions().getRounds());

        ui->labelAmountRounds->setToolTip(tr("No inputs detected"));
        ui->labelAmountRounds->setText(strAmountAndRounds);
        return;
    }

    CAmount nAnonymizableBalance = walletModel->wallet().getAnonymizableBalance(false, false);

    CAmount nMaxToAnonymize = nAnonymizableBalance + m_balances.anonymized_balance;

    // If it's more than the anon threshold, limit to that.
    if (nMaxToAnonymize > clientModel->coinJoinOptions().getAmount() * COIN) nMaxToAnonymize = clientModel->coinJoinOptions().getAmount() * COIN;

    if(nMaxToAnonymize == 0) return;

    if (nMaxToAnonymize >= clientModel->coinJoinOptions().getAmount() * COIN) {
        ui->labelAmountRounds->setToolTip(tr("Found enough compatible inputs to mix %1")
                                          .arg(strCoinJoinAmount));
        strCoinJoinAmount = strCoinJoinAmount.remove(strCoinJoinAmount.indexOf("."), BitcoinUnits::decimals(nDisplayUnit) + 1);
        strAmountAndRounds = strCoinJoinAmount + " / " + tr("%n Rounds", "", clientModel->coinJoinOptions().getRounds());
    } else {
        QString strMaxToAnonymize = BitcoinUnits::formatHtmlWithUnit(nDisplayUnit, nMaxToAnonymize, false, BitcoinUnits::separatorAlways);
        ui->labelAmountRounds->setToolTip(tr("Not enough compatible inputs to mix <span style='%1'>%2</span>,<br>"
                                             "will mix <span style='%1'>%3</span> instead")
                                          .arg(GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_ERROR))
                                          .arg(strCoinJoinAmount)
                                          .arg(strMaxToAnonymize));
        strMaxToAnonymize = strMaxToAnonymize.remove(strMaxToAnonymize.indexOf("."), BitcoinUnits::decimals(nDisplayUnit) + 1);
        strAmountAndRounds = "<span style='" + GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_ERROR) + "'>" +
                QString(BitcoinUnits::factor(nDisplayUnit) == 1 ? "" : "~") + strMaxToAnonymize +
                " / " + tr("%n Rounds", "", clientModel->coinJoinOptions().getRounds()) + "</span>";
    }
    ui->labelAmountRounds->setText(strAmountAndRounds);

    if (!fShowAdvancedCJUI) return;

    CAmount nDenominatedConfirmedBalance;
    CAmount nDenominatedUnconfirmedBalance;
    CAmount nNormalizedAnonymizedBalance;
    float nAverageAnonymizedRounds;

    nDenominatedConfirmedBalance = walletModel->wallet().getDenominatedBalance(false);
    nDenominatedUnconfirmedBalance = walletModel->wallet().getDenominatedBalance(true);
    nNormalizedAnonymizedBalance = walletModel->wallet().getNormalizedAnonymizedBalance();
    nAverageAnonymizedRounds = walletModel->wallet().getAverageAnonymizedRounds();

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

    anonFullPart = (float)m_balances.anonymized_balance / nMaxToAnonymize;
    anonFullPart = anonFullPart > 1 ? 1 : anonFullPart;
    anonFullPart *= 100;

    // apply some weights to them ...
    float denomWeight = 1;
    float anonNormWeight = clientModel->coinJoinOptions().getRounds();
    float anonFullWeight = 2;
    float fullWeight = denomWeight + anonNormWeight + anonFullWeight;
    // ... and calculate the whole progress
    float denomPartCalc = ceilf((denomPart * denomWeight / fullWeight) * 100) / 100;
    float anonNormPartCalc = ceilf((anonNormPart * anonNormWeight / fullWeight) * 100) / 100;
    float anonFullPartCalc = ceilf((anonFullPart * anonFullWeight / fullWeight) * 100) / 100;
    float progress = denomPartCalc + anonNormPartCalc + anonFullPartCalc;
    if(progress >= 100) progress = 100;

    ui->coinJoinProgress->setValue(progress);

    QString strToolPip = ("<b>" + tr("Overall progress") + ": %1%</b><br/>" +
                          tr("Denominated") + ": %2%<br/>" +
                          tr("Partially mixed") + ": %3%<br/>" +
                          tr("Mixed") + ": %4%<br/>" +
                          tr("Denominated inputs have %5 of %n rounds on average", "", clientModel->coinJoinOptions().getRounds()))
            .arg(progress).arg(denomPart).arg(anonNormPart).arg(anonFullPart)
            .arg(nAverageAnonymizedRounds);
    ui->coinJoinProgress->setToolTip(strToolPip);
}

void OverviewPage::updateAdvancedCJUI(bool fShowAdvancedCJUI)
{
    if (!walletModel || !clientModel) return;

    this->fShowAdvancedCJUI = fShowAdvancedCJUI;
    coinJoinStatus(true);
}

void OverviewPage::coinJoinStatus(bool fForce)
{
    if (!walletModel || !clientModel) return;

    if (!fForce && (clientModel->node().shutdownRequested() || !clientModel->masternodeSync().isBlockchainSynced())) return;

    // Disable any PS UI for masternode or when autobackup is disabled or failed for whatever reason
    if (fMasternodeMode || nWalletBackups <= 0) {
        DisableCoinJoinCompletely();
        if (nWalletBackups <= 0) {
            ui->labelCoinJoinEnabled->setToolTip(tr("Automatic backups are disabled, no mixing available!"));
        }
        return;
    }

    bool fIsEnabled = clientModel->coinJoinOptions().isEnabled();
    ui->frameCoinJoin->setVisible(fIsEnabled);
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

    // Wrap all coinjoin related widgets we want to show/hide state based.
    // Value of the map contains a flag if this widget belongs to the advanced
    // CoinJoin UI option or not. True if it does, false if not.
    std::map<QWidget*, bool> coinJoinWidgets = {
        {ui->labelCompletitionText, true},
        {ui->coinJoinProgress, true},
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
        if (fLastShowAdvanced == fShowAdvancedCJUI && fLastVisible == fVisible) {
            if (fInitial) {
                fInitial = false;
            } else {
                return;
            }
        }
        // Set visible if: fVisible and not advanced UI element or advanced ui element and advanced ui active
        for (const auto& it : coinJoinWidgets) {
            it.first->setVisible(fVisible && (!it.second || it.second == fShowAdvancedCJUI));
        }
        fLastVisible = fVisible;
        fLastShowAdvanced = fShowAdvancedCJUI;
    };

    static int64_t nLastDSProgressBlockTime = 0;
    int nBestHeight = clientModel->node().getNumBlocks();

    // We are processing more than 1 block per second, we'll just leave
    if (nBestHeight > walletModel->coinJoin().getCachedBlocks() && GetTime() - nLastDSProgressBlockTime <= 1) return;
    nLastDSProgressBlockTime = GetTime();

    QString strKeysLeftText(tr("keys left: %1").arg(walletModel->getKeysLeftSinceAutoBackup()));
    if(walletModel->getKeysLeftSinceAutoBackup() < COINJOIN_KEYS_THRESHOLD_WARNING) {
        strKeysLeftText = "<span style='" + GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_ERROR) + "'>" + strKeysLeftText + "</span>";
    }
    ui->labelCoinJoinEnabled->setToolTip(strKeysLeftText);

    if (!walletModel->coinJoin().isMixing()) {
        if (nBestHeight != walletModel->coinJoin().getCachedBlocks()) {
            walletModel->coinJoin().setCachedBlocks(nBestHeight);
            updateCoinJoinProgress();
        }

        setWidgetsVisible(false);
        ui->toggleCoinJoin->setText(tr("Start %1").arg("CoinJoin"));

        QString strEnabled = tr("Disabled");
        // Show how many keys left in advanced PS UI mode only
        if (fShowAdvancedCJUI) strEnabled += ", " + strKeysLeftText;
        ui->labelCoinJoinEnabled->setText(strEnabled);

        // If mixing isn't active always show the lower number of txes because there are
        // anyway the most PS widgets hidden.
        SetupTransactionList(NUM_ITEMS_ENABLED_NORMAL);

        return;
    } else {
        SetupTransactionList(fShowAdvancedCJUI ? NUM_ITEMS_ENABLED_ADVANCED : NUM_ITEMS_ENABLED_NORMAL);
    }

    // Warn user that wallet is running out of keys
    // NOTE: we do NOT warn user and do NOT create autobackups if mixing is not running
    if (nWalletBackups > 0 && walletModel->getKeysLeftSinceAutoBackup() < COINJOIN_KEYS_THRESHOLD_WARNING) {
        QSettings settings;
        if(settings.value("fLowKeysWarning").toBool()) {
            QString strWarn =   tr("Very low number of keys left since last automatic backup!") + "<br><br>" +
                                tr("We are about to create a new automatic backup for you, however "
                                   "<span style='%1'> you should always make sure you have backups "
                                   "saved in some safe place</span>!").arg(GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_COMMAND)) + "<br><br>" +
                                tr("Note: You can turn this message off in options.");
            ui->labelCoinJoinEnabled->setToolTip(strWarn);
            LogPrint(BCLog::COINJOIN, "OverviewPage::coinJoinStatus -- Very low number of keys left since last automatic backup, warning user and trying to create new backup...\n");
            QMessageBox::warning(this, "CoinJoin", strWarn, QMessageBox::Ok, QMessageBox::Ok);
        } else {
            LogPrint(BCLog::COINJOIN, "OverviewPage::coinJoinStatus -- Very low number of keys left since last automatic backup, skipping warning and trying to create new backup...\n");
        }

        QString strBackupWarning;
        QString strBackupError;
        if(!walletModel->autoBackupWallet(strBackupWarning, strBackupError)) {
            if (!strBackupWarning.isEmpty()) {
                // It's still more or less safe to continue but warn user anyway
                LogPrint(BCLog::COINJOIN, "OverviewPage::coinJoinStatus -- WARNING! Something went wrong on automatic backup: %s\n", strBackupWarning.toStdString());

                QMessageBox::warning(this, "CoinJoin",
                    tr("WARNING! Something went wrong on automatic backup") + ":<br><br>" + strBackupWarning,
                    QMessageBox::Ok, QMessageBox::Ok);
            }
            if (!strBackupError.isEmpty()) {
                // Things are really broken, warn user and stop mixing immediately
                LogPrint(BCLog::COINJOIN, "OverviewPage::coinJoinStatus -- ERROR! Failed to create automatic backup: %s\n", strBackupError.toStdString());

                QMessageBox::warning(this, "CoinJoin",
                    tr("ERROR! Failed to create automatic backup") + ":<br><br>" + strBackupError + "<br>" +
                    tr("Mixing is disabled, please close your wallet and fix the issue!"),
                    QMessageBox::Ok, QMessageBox::Ok);
            }
        }
    }

    QString strEnabled = walletModel->coinJoin().isMixing() ? tr("Enabled") : tr("Disabled");
    // Show how many keys left in advanced PS UI mode only
    if(fShowAdvancedCJUI) strEnabled += ", " + strKeysLeftText;
    ui->labelCoinJoinEnabled->setText(strEnabled);

    if(nWalletBackups == -1) {
        // Automatic backup failed, nothing else we can do until user fixes the issue manually
        DisableCoinJoinCompletely();

        QString strError =  tr("ERROR! Failed to create automatic backup") + ", " +
                            tr("see debug.log for details.") + "<br><br>" +
                            tr("Mixing is disabled, please close your wallet and fix the issue!");
        ui->labelCoinJoinEnabled->setToolTip(strError);

        return;
    } else if(nWalletBackups == -2) {
        // We were able to create automatic backup but keypool was not replenished because wallet is locked.
        QString strWarning = tr("WARNING! Failed to replenish keypool, please unlock your wallet to do so.");
        ui->labelCoinJoinEnabled->setToolTip(strWarning);
    }

    // check coinjoin status and unlock if needed
    if(nBestHeight != walletModel->coinJoin().getCachedBlocks()) {
        // Balance and number of transactions might have changed
        walletModel->coinJoin().setCachedBlocks(nBestHeight);
        updateCoinJoinProgress();
    }

    setWidgetsVisible(true);

    ui->labelSubmittedDenom->setText(QString(walletModel->coinJoin().getSessionDenoms().c_str()));
}

void OverviewPage::toggleCoinJoin(){
    QSettings settings;
    // Popup some information on first mixing
    QString hasMixed = settings.value("hasMixed").toString();
    if(hasMixed.isEmpty()){
        QMessageBox::information(this, "CoinJoin",
                tr("If you don't want to see internal %1 fees/transactions select \"Most Common\" as Type on the \"Transactions\" tab.").arg("CoinJoin"),
                QMessageBox::Ok, QMessageBox::Ok);
        settings.setValue("hasMixed", "hasMixed");
    }

    if (!walletModel->coinJoin().isMixing()) {
        auto& options = walletModel->node().coinJoinOptions();
        const CAmount nMinAmount = options.getSmallestDenomination() + options.getMaxCollateralAmount();
        if(m_balances.balance < nMinAmount) {
            QString strMinAmount(BitcoinUnits::formatWithUnit(nDisplayUnit, nMinAmount));
            QMessageBox::warning(this, "CoinJoin",
                tr("%1 requires at least %2 to use.").arg("CoinJoin").arg(strMinAmount),
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
                walletModel->coinJoin().resetCachedBlocks();
                QMessageBox::warning(this, "CoinJoin",
                    tr("Wallet is locked and user declined to unlock. Disabling %1.").arg("CoinJoin"),
                    QMessageBox::Ok, QMessageBox::Ok);
                LogPrint(BCLog::COINJOIN, "OverviewPage::toggleCoinJoin -- Wallet is locked and user declined to unlock. Disabling CoinJoin.\n");
                return;
            }
        }

    }

    walletModel->coinJoin().resetCachedBlocks();

    if (walletModel->coinJoin().isMixing()) {
        ui->toggleCoinJoin->setText(tr("Start %1").arg("CoinJoin"));
        walletModel->coinJoin().resetPool();
        walletModel->coinJoin().stopMixing();
    } else {
        ui->toggleCoinJoin->setText(tr("Stop %1").arg("CoinJoin"));
        walletModel->coinJoin().startMixing();
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

void OverviewPage::DisableCoinJoinCompletely()
{
    if (walletModel == nullptr) {
        return;
    }

    ui->toggleCoinJoin->setText("(" + tr("Disabled") + ")");
    ui->frameCoinJoin->setEnabled(false);
    if (nWalletBackups <= 0) {
        ui->labelCoinJoinEnabled->setText("<span style='" + GUIUtil::getThemedStyleQString(GUIUtil::ThemedStyle::TS_ERROR) + "'>(" + tr("Disabled") + ")</span>");
    }
    walletModel->coinJoin().stopMixing();
}
