// Copyright (c) 2011-2015 The LibertaCore developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "libertaunits.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "init.h"
#include "obfuscation.h"
#include "obfuscationconfig.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "transactionfilterproxy.h"
#include "transactiontablemodel.h"
#include "walletmodel.h"

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QSettings>
#include <QTimer>

#define DECORATION_SIZE 54
#define NUM_ITEMS 5

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate(const PlatformStyle *platformStyle):
        QAbstractItemDelegate(), unit(LibertaUnits::LBT),
        platformStyle(platformStyle)
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
        QString amountText = LibertaUnits::formatWithUnit(unit, amount, true, LibertaUnits::separatorAlways);
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
    const PlatformStyle *platformStyle;

};
#include "overviewpage.moc"

OverviewPage::OverviewPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage),
    clientModel(0),
    walletModel(0),
    currentBalance(-1),
    currentStake(0),
    currentUnconfirmedBalance(-1),
    currentImmatureBalance(-1),
    currentWatchOnlyBalance(-1),
    currentWatchUnconfBalance(-1),
    currentWatchImmatureBalance(-1),
    txdelegate(new TxViewDelegate(platformStyle)),
    filter(0)
{
    ui->setupUi(this);

    // use a SingleColorIcon for the "out of sync warning" icon
    QIcon icon = platformStyle->SingleColorIcon(":/icons/warning");
    icon.addPixmap(icon.pixmap(QSize(64,64), QIcon::Normal), QIcon::Disabled); // also set the disabled icon because we are using a disabled QPushButton to work around missing HiDPI support of QLabel (https://bugreports.qt.io/browse/QTBUG-42503)
    ui->labelTransactionsStatus->setIcon(icon);
    ui->labelWalletStatus->setIcon(icon);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));

    ui->labelObfuscationSyncStatus->setText("(" + tr("out of sync") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("out of sync") + ")");

    if (fLiteMode) {
        ui->frameObfuscation->setVisible(false);
    } else {
        if (fMasterNode) {
            ui->toggleObfuscation->setText("(" + tr("Disabled") + ")");
            ui->obfuscationAuto->setText("(" + tr("Disabled") + ")");
            ui->obfuscationReset->setText("(" + tr("Disabled") + ")");
            ui->frameObfuscation->setEnabled(false);
        } else {
            if (!fEnableObfuscation) {
                ui->toggleObfuscation->setText(tr("Start Obfuscation"));
            } else {
                ui->toggleObfuscation->setText(tr("Stop Obfuscation"));
            }
            timer = new QTimer(this);
            connect(timer, SIGNAL(timeout()), this, SLOT(obfuScationStatus()));
            timer->start(1000);
        }
    }

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);
}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        Q_EMIT transactionClicked(filter->mapToSource(index));
}

OverviewPage::~OverviewPage()
{
    if (!fLiteMode && !fMasterNode) disconnect(timer, SIGNAL(timeout()), this, SLOT(obfuScationStatus()));
    delete ui;
}

void OverviewPage::setBalance(const CAmount& balance, const CAmount& stake, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance, const CAmount& anonymizedBalance)
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentStake = stake;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentAnonymizedBalance = anonymizedBalance;
    currentWatchOnlyBalance = watchOnlyBalance;
    currentWatchUnconfBalance = watchUnconfBalance;
    currentWatchImmatureBalance = watchImmatureBalance;
    ui->labelBalance->setText(LibertaUnits::formatWithUnit(unit, balance, false, LibertaUnits::separatorAlways));
    ui->labelStake->setText(LibertaUnits::formatWithUnit(unit, stake, false, LibertaUnits::separatorAlways));
    ui->labelUnconfirmed->setText(LibertaUnits::formatWithUnit(unit, unconfirmedBalance, false, LibertaUnits::separatorAlways));
    ui->labelImmature->setText(LibertaUnits::formatWithUnit(unit, immatureBalance, false, LibertaUnits::separatorAlways));
    ui->labelAnonymized->setText(LibertaUnits::formatWithUnit(unit, anonymizedBalance, false, LibertaUnits::separatorAlways));
    ui->labelTotal->setText(LibertaUnits::formatWithUnit(unit, balance + stake + unconfirmedBalance + immatureBalance, false, LibertaUnits::separatorAlways));
    //ui->labelTotal->setText(LibertaUnits::formatWithUnit(unit, balance + unconfirmedBalance + immatureBalance, false, LibertaUnits::separatorAlways));

    ui->labelWatchAvailable->setText(LibertaUnits::formatWithUnit(unit, watchOnlyBalance, false, LibertaUnits::separatorAlways));
    ui->labelWatchPending->setText(LibertaUnits::formatWithUnit(unit, watchUnconfBalance, false, LibertaUnits::separatorAlways));
    ui->labelWatchImmature->setText(LibertaUnits::formatWithUnit(unit, watchImmatureBalance, false, LibertaUnits::separatorAlways));
    ui->labelWatchTotal->setText(LibertaUnits::formatWithUnit(unit, watchOnlyBalance + watchUnconfBalance + watchImmatureBalance, false, LibertaUnits::separatorAlways));

    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = immatureBalance != 0;
    bool showWatchOnlyImmature = watchImmatureBalance != 0;

    // for symmetry reasons also show immature label when the watch-only one is shown
    ui->labelImmature->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelImmatureText->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelWatchImmature->setVisible(showWatchOnlyImmature); // show watch-only immature balance

    updateObfuscationProgress();
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
        filter = new TransactionFilterProxy();
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Status, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter);
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getStake(), model->getUnconfirmedBalance(), model->getImmatureBalance(),
            model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance(), model->getAnonymizedBalance());
        connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount, CAmount, CAmount,CAmount)), this, SLOT(setBalance(CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount,CAmount)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        connect(ui->obfuscationAuto, SIGNAL(clicked()), this, SLOT(obfuscationAuto()));
        connect(ui->obfuscationReset, SIGNAL(clicked()), this, SLOT(obfuscationReset()));
        connect(ui->toggleObfuscation, SIGNAL(clicked()), this, SLOT(toggleObfuscation()));
        updateWatchOnlyLabels(model->haveWatchOnly());
        connect(model, SIGNAL(notifyWatchonlyChanged(bool)), this, SLOT(updateWatchOnlyLabels(bool)));
    }

    // update the display unit, to not use the default ("LBT")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        if(currentBalance != -1)
            setBalance(currentBalance, walletModel->getStake(), currentUnconfirmedBalance, currentImmatureBalance,
                currentWatchOnlyBalance, currentWatchUnconfBalance, currentWatchImmatureBalance, currentAnonymizedBalance);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = walletModel->getOptionsModel()->getDisplayUnit();
        nDisplayUnit =txdelegate->unit;

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
    ui->labelObfuscationSyncStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}

void OverviewPage::updateObfuscationProgress()
{
    if (!masternodeSync.IsBlockchainSynced() || ShutdownRequested()) return;

    if (!pwalletMain) return;

    QString strAmountAndRounds;
    QString strAnonymizeLibertaAmount = LibertaUnits::formatHtmlWithUnit(nDisplayUnit, nAnonymizeLibertaAmount * COIN, false, LibertaUnits::separatorAlways);

    if (currentBalance == 0) {
        ui->obfuscationProgress->setValue(0);
        ui->obfuscationProgress->setToolTip(tr("No inputs detected"));

        // when balance is zero just show info from settings
        strAnonymizeLibertaAmount = strAnonymizeLibertaAmount.remove(strAnonymizeLibertaAmount.indexOf("."), LibertaUnits::decimals(nDisplayUnit) + 1);
        strAmountAndRounds = strAnonymizeLibertaAmount + " / " + tr("%n Rounds", "", nObfuscationRounds);

        ui->labelAmountRounds->setToolTip(tr("No inputs detected"));
        ui->labelAmountRounds->setText(strAmountAndRounds);
        return;
    }

    CAmount nDenominatedConfirmedBalance;
    CAmount nDenominatedUnconfirmedBalance;
    CAmount nAnonymizableBalance;
    CAmount nNormalizedAnonymizedBalance;
    double nAverageAnonymizedRounds;

    {
        TRY_LOCK(cs_main, lockMain);
        if (!lockMain) return;

        nDenominatedConfirmedBalance = pwalletMain->GetDenominatedBalance();
        nDenominatedUnconfirmedBalance = pwalletMain->GetDenominatedBalance(true);
        nAnonymizableBalance = pwalletMain->GetAnonymizableBalance();
        nNormalizedAnonymizedBalance = pwalletMain->GetNormalizedAnonymizedBalance();
        nAverageAnonymizedRounds = pwalletMain->GetAverageAnonymizedRounds();
    }

    CAmount nMaxToAnonymize = nAnonymizableBalance + currentAnonymizedBalance + nDenominatedUnconfirmedBalance;

    // If it's more than the anon threshold, limit to that.
    if (nMaxToAnonymize > nAnonymizeLibertaAmount * COIN) nMaxToAnonymize = nAnonymizeLibertaAmount * COIN;

    if (nMaxToAnonymize == 0) return;

    if (nMaxToAnonymize >= nAnonymizeLibertaAmount * COIN) {
        ui->labelAmountRounds->setToolTip(tr("Found enough compatible inputs to anonymize %1")
                                              .arg(strAnonymizeLibertaAmount));
        strAnonymizeLibertaAmount = strAnonymizeLibertaAmount.remove(strAnonymizeLibertaAmount.indexOf("."), LibertaUnits::decimals(nDisplayUnit) + 1);
        strAmountAndRounds = strAnonymizeLibertaAmount + " / " + tr("%n Rounds", "", nObfuscationRounds);
    } else {
        QString strMaxToAnonymize = LibertaUnits::formatHtmlWithUnit(nDisplayUnit, nMaxToAnonymize, false, LibertaUnits::separatorAlways);
        ui->labelAmountRounds->setToolTip(tr("Not enough compatible inputs to anonymize <span style='color:red;'>%1</span>,<br>"
                                             "will anonymize <span style='color:red;'>%2</span> instead")
                                              .arg(strAnonymizeLibertaAmount)
                                              .arg(strMaxToAnonymize));
        strMaxToAnonymize = strMaxToAnonymize.remove(strMaxToAnonymize.indexOf("."), LibertaUnits::decimals(nDisplayUnit) + 1);
        strAmountAndRounds = "<span style='color:red;'>" +
                             QString(LibertaUnits::factor(nDisplayUnit) == 1 ? "" : "~") + strMaxToAnonymize +
                             " / " + tr("%n Rounds", "", nObfuscationRounds) + "</span>";
    }
    ui->labelAmountRounds->setText(strAmountAndRounds);

    // calculate parts of the progress, each of them shouldn't be higher than 1
    // progress of denominating
    float denomPart = 0;
    // mixing progress of denominated balance
    float anonNormPart = 0;
    // completeness of full amount anonimization
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
    float anonNormWeight = nObfuscationRounds;
    float anonFullWeight = 2;
    float fullWeight = denomWeight + anonNormWeight + anonFullWeight;
    // ... and calculate the whole progress
    float denomPartCalc = ceilf((denomPart * denomWeight / fullWeight) * 100) / 100;
    float anonNormPartCalc = ceilf((anonNormPart * anonNormWeight / fullWeight) * 100) / 100;
    float anonFullPartCalc = ceilf((anonFullPart * anonFullWeight / fullWeight) * 100) / 100;
    float progress = denomPartCalc + anonNormPartCalc + anonFullPartCalc;
    if (progress >= 100) progress = 100;

    ui->obfuscationProgress->setValue(progress);

    QString strToolPip = ("<b>" + tr("Overall progress") + ": %1%</b><br/>" +
                          tr("Denominated") + ": %2%<br/>" +
                          tr("Mixed") + ": %3%<br/>" +
                          tr("Anonymized") + ": %4%<br/>" +
                          tr("Denominated inputs have %5 of %n rounds on average", "", nObfuscationRounds))
                             .arg(progress)
                             .arg(denomPart)
                             .arg(anonNormPart)
                             .arg(anonFullPart)
                             .arg(nAverageAnonymizedRounds);
    ui->obfuscationProgress->setToolTip(strToolPip);
}


void OverviewPage::obfuScationStatus()
{
    static int64_t nLastDSProgressBlockTime = 0;

    int nBestHeight = chainActive.Tip()->nHeight;

    // we we're processing more then 1 block per second, we'll just leave
    if (((nBestHeight - obfuScationPool.cachedNumBlocks) / (GetTimeMillis() - nLastDSProgressBlockTime + 1) > 1)) return;
    nLastDSProgressBlockTime = GetTimeMillis();

    if (!fEnableObfuscation) {
        if (nBestHeight != obfuScationPool.cachedNumBlocks) {
            obfuScationPool.cachedNumBlocks = nBestHeight;
            updateObfuscationProgress();

            ui->obfuscationEnabled->setText(tr("Disabled"));
            ui->obfuscationStatus->setText("");
            ui->toggleObfuscation->setText(tr("Start Obfuscation"));
        }

        return;
    }

    // check obfuscation status and unlock if needed
    if (nBestHeight != obfuScationPool.cachedNumBlocks) {
        // Balance and number of transactions might have changed
        obfuScationPool.cachedNumBlocks = nBestHeight;
        updateObfuscationProgress();

        ui->obfuscationEnabled->setText(tr("Enabled"));
    }

    QString strStatus = QString(obfuScationPool.GetStatus().c_str());

    QString s = tr("Last Obfuscation message:\n") + strStatus;

    if (s != ui->obfuscationStatus->text())
        LogPrintf("Last Obfuscation message: %s\n", strStatus.toStdString());

    ui->obfuscationStatus->setText(s);

    if (obfuScationPool.sessionDenom == 0) {
        ui->labelSubmittedDenom->setText(tr("N/A"));
    } else {
        std::string out;
        obfuScationPool.GetDenominationsToString(obfuScationPool.sessionDenom, out);
        QString s2(out.c_str());
        ui->labelSubmittedDenom->setText(s2);
    }
}

void OverviewPage::obfuscationAuto()
{
    obfuScationPool.DoAutomaticDenominating();
}

void OverviewPage::obfuscationReset()
{
    obfuScationPool.Reset();

    QMessageBox::warning(this, tr("Obfuscation"),
        tr("Obfuscation was successfully reset."),
        QMessageBox::Ok, QMessageBox::Ok);
}

void OverviewPage::toggleObfuscation()
{
    QSettings settings;
    // Popup some information on first mixing
    QString hasMixed = settings.value("hasMixed").toString();
    if (hasMixed.isEmpty()) {
        QMessageBox::information(this, tr("Obfuscation"),
            tr("If you don't want to see internal Obfuscation fees/transactions select \"Most Common\" as Type on the \"Transactions\" tab."),
            QMessageBox::Ok, QMessageBox::Ok);
        settings.setValue("hasMixed", "hasMixed");
    }
    if (!fEnableObfuscation) {
        int64_t balance = currentBalance;
        float minAmount = 14.90 * COIN;
        if (balance < minAmount) {
            QString strMinAmount(LibertaUnits::formatWithUnit(nDisplayUnit, minAmount));
            QMessageBox::warning(this, tr("Obfuscation"),
                tr("Obfuscation requires at least %1 to use.").arg(strMinAmount),
                QMessageBox::Ok, QMessageBox::Ok);
            return;
        }

        // if wallet is locked, ask for a passphrase
        if (walletModel->getEncryptionStatus() == WalletModel::Locked) {
            WalletModel::UnlockContext ctx(walletModel->requestUnlock());
            if (!ctx.isValid()) {
                //unlock was cancelled
                obfuScationPool.cachedNumBlocks = std::numeric_limits<int>::max();
                QMessageBox::warning(this, tr("Obfuscation"),
                    tr("Wallet is locked and user declined to unlock. Disabling Obfuscation."),
                    QMessageBox::Ok, QMessageBox::Ok);
                if (fDebug) LogPrintf("Wallet is locked and user declined to unlock. Disabling Obfuscation.\n");
                return;
            }
        }
    }

    fEnableObfuscation = !fEnableObfuscation;
    obfuScationPool.cachedNumBlocks = std::numeric_limits<int>::max();

    if (!fEnableObfuscation) {
        ui->toggleObfuscation->setText(tr("Start Obfuscation"));
        obfuScationPool.UnlockCoins();
    } else {
        ui->toggleObfuscation->setText(tr("Stop Obfuscation"));

        /* show obfuscation configuration if client has defaults set */

        if (nAnonymizeLibertaAmount == 0) {
            ObfuscationConfig dlg(this);
            dlg.setModel(walletModel);
            dlg.exec();
        }
    }
}
