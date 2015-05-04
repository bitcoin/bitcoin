// Copyright (c) 2011-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "bitcoinunits.h"
#include "clientmodel.h"
#include "darksend.h"
#include "darksendconfig.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "transactionfilterproxy.h"
#include "transactiontablemodel.h"
#include "walletmodel.h"
#include "init.h"

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QTimer>

#define DECORATION_SIZE 48
#define ICON_OFFSET 16
#define NUM_ITEMS 5

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate(): QAbstractItemDelegate(), unit(BitcoinUnits::DASH)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        QRect mainRect = option.rect;
        mainRect.moveLeft(ICON_OFFSET);
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace - ICON_OFFSET, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);
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
        QString amountText = BitcoinUnits::formatWithUnit(unit, amount, true, BitcoinUnits::separatorAlways);
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
    walletModel(0),
    currentBalance(-1),
    currentUnconfirmedBalance(-1),
    currentImmatureBalance(-1),
    currentWatchOnlyBalance(-1),
    currentWatchUnconfBalance(-1),
    currentWatchImmatureBalance(-1),
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
    ui->labelDarksendSyncStatus->setText("(" + tr("out of sync") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("out of sync") + ")");

    showingDarkSendMessage = 0;
    darksendActionCheck = 0;
    lastNewBlock = 0;

    if(fLiteMode){
        ui->frameDarksend->setVisible(false);
    } else if(!fMasterNode) {
        timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(darkSendStatus()));
        timer->start(333);
    }

    if(fMasterNode){
        ui->toggleDarksend->setText("(" + tr("Disabled") + ")");
        ui->darksendAuto->setText("(" + tr("Disabled") + ")");
        ui->darksendReset->setText("(" + tr("Disabled") + ")");
        ui->frameDarksend->setEnabled(false);
    }else if(!fEnableDarksend){
        ui->toggleDarksend->setText(tr("Start Darksend Mixing"));
    } else {
        ui->toggleDarksend->setText(tr("Stop Darksend Mixing"));
    }

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
    if(!fLiteMode && !fMasterNode) disconnect(timer, SIGNAL(timeout()), this, SLOT(darkSendStatus()));
    delete ui;
}

void OverviewPage::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, const CAmount& anonymizedBalance, const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance)
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentAnonymizedBalance = anonymizedBalance;
    currentWatchOnlyBalance = watchOnlyBalance;
    currentWatchUnconfBalance = watchUnconfBalance;
    currentWatchImmatureBalance = watchImmatureBalance;
    ui->labelBalance->setText(BitcoinUnits::formatWithUnit(unit, balance, false, BitcoinUnits::separatorAlways));
    ui->labelUnconfirmed->setText(BitcoinUnits::formatWithUnit(unit, unconfirmedBalance, false, BitcoinUnits::separatorAlways));
    ui->labelImmature->setText(BitcoinUnits::formatWithUnit(unit, immatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelAnonymized->setText(BitcoinUnits::formatWithUnit(unit, anonymizedBalance, false, BitcoinUnits::separatorAlways));
    ui->labelTotal->setText(BitcoinUnits::formatWithUnit(unit, balance + unconfirmedBalance + immatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchAvailable->setText(BitcoinUnits::formatWithUnit(unit, watchOnlyBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchPending->setText(BitcoinUnits::formatWithUnit(unit, watchUnconfBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchImmature->setText(BitcoinUnits::formatWithUnit(unit, watchImmatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchTotal->setText(BitcoinUnits::formatWithUnit(unit, watchOnlyBalance + watchUnconfBalance + watchImmatureBalance, false, BitcoinUnits::separatorAlways));

    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = immatureBalance != 0;
    bool showWatchOnlyImmature = watchImmatureBalance != 0;

    // for symmetry reasons also show immature label when the watch-only one is shown
    ui->labelImmature->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelImmatureText->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelWatchImmature->setVisible(showWatchOnlyImmature); // show watch-only immature balance

    if(cachedTxLocks != nCompleteTXLocks){
        cachedTxLocks = nCompleteTXLocks;
        ui->listTransactions->update();
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
        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(), model->getAnonymizedBalance(),
                   model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance());
        connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(setBalance(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        connect(ui->darksendAuto, SIGNAL(clicked()), this, SLOT(darksendAuto()));
        connect(ui->darksendReset, SIGNAL(clicked()), this, SLOT(darksendReset()));
        connect(ui->toggleDarksend, SIGNAL(clicked()), this, SLOT(toggleDarksend()));
        updateWatchOnlyLabels(model->haveWatchOnly());
        connect(model, SIGNAL(notifyWatchonlyChanged(bool)), this, SLOT(updateWatchOnlyLabels(bool)));
    }

    // update the display unit, to not use the default ("DASH")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        if(currentBalance != -1)
            setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance, currentAnonymizedBalance,
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
    ui->labelDarksendSyncStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}

void OverviewPage::updateDarksendProgress()
{
    if(IsInitialBlockDownload()) return;

    int64_t nBalance = pwalletMain->GetBalance();
    if(nBalance == 0)
    {
        ui->darksendProgress->setValue(0);
        QString s(tr("No inputs detected"));
        ui->darksendProgress->setToolTip(s);

        // when balance is zero just show info from settings
        QString strSettings = BitcoinUnits::formatWithUnit(
                    walletModel->getOptionsModel()->getDisplayUnit(),
                    nAnonymizeDarkcoinAmount * COIN
                ) + " / " + tr("%n Rounds", "", nDarksendRounds);

        ui->labelAmountRounds->setText(strSettings);
        return;
    }

    //get denominated unconfirmed inputs
    if(pwalletMain->GetDenominatedBalance(true, true) > 0)
    {
        QString s(tr("Found unconfirmed denominated outputs, will wait till they confirm to recalculate."));
        ui->darksendProgress->setToolTip(s);
        return;
    }

    int64_t nMaxToAnonymize = pwalletMain->GetAnonymizableBalance(true);

    // If it's more than the anon threshold, limit to that.
    if(nMaxToAnonymize > nAnonymizeDarkcoinAmount*COIN) nMaxToAnonymize = nAnonymizeDarkcoinAmount*COIN;

    if(nMaxToAnonymize == 0) return;

    // calculate parts of the progress, each of them shouldn't be higher than 1:
    // mixing progress of denominated balance
    int64_t denominatedBalance = pwalletMain->GetDenominatedBalance();
    float denomPart = 0;
    if(denominatedBalance > 0)
    {
        denomPart = (float)pwalletMain->GetNormalizedAnonymizedBalance() / denominatedBalance;
        denomPart = denomPart > 1 ? 1 : denomPart;
    }

    // % of fully anonymized balance
    float anonPart = 0;
    if(nMaxToAnonymize > 0)
    {
        anonPart = (float)pwalletMain->GetAnonymizedBalance() / nMaxToAnonymize;
        // if anonPart is > 1 then we are done, make denomPart equal 1 too
        anonPart = anonPart > 1 ? (denomPart = 1, 1) : anonPart;
    }

    // apply some weights to them (sum should be <=100) and calculate the whole progress
    int progress = 80 * denomPart + 20 * anonPart;
    if(progress >= 100) progress = 100;

    ui->darksendProgress->setValue(progress);

    QString strToolPip = tr("Progress: %1% (inputs have an average of %2 of %n rounds)", "", nDarksendRounds).arg(progress).arg(pwalletMain->GetAverageAnonymizedRounds());
    ui->darksendProgress->setToolTip(strToolPip);

    QString strSettings;
    if(nMaxToAnonymize >= nAnonymizeDarkcoinAmount * COIN) {
        ui->labelAmountRounds->setToolTip(tr("Found enough compatible inputs to anonymize %1")
                                          .arg(BitcoinUnits::formatWithUnit(
                                                   walletModel->getOptionsModel()->getDisplayUnit(),
                                                   nAnonymizeDarkcoinAmount * COIN
                                               )));
        strSettings = BitcoinUnits::formatWithUnit(
                    walletModel->getOptionsModel()->getDisplayUnit(),
                    nAnonymizeDarkcoinAmount * COIN
                ) + " / " + tr("%n Rounds", "", nDarksendRounds);
    } else {
        ui->labelAmountRounds->setToolTip(tr("Not enough compatible inputs to anonymize <span style='color:red;'>%1</span>,<br/>"
                                             "will anonymize <span style='color:red;'>%2</span> instead")
                                          .arg(BitcoinUnits::formatWithUnit(
                                                   walletModel->getOptionsModel()->getDisplayUnit(),
                                                   nAnonymizeDarkcoinAmount * COIN
                                               ))
                                          .arg(BitcoinUnits::formatWithUnit(
                                                   walletModel->getOptionsModel()->getDisplayUnit(),
                                                   nMaxToAnonymize
                                               )));
        strSettings = "<span style='color:red;'>" + BitcoinUnits::formatWithUnit(
                    walletModel->getOptionsModel()->getDisplayUnit(),
                    nMaxToAnonymize
                ) + " / " + tr("%n Rounds", "", nDarksendRounds) + "</span>";
    }

    ui->labelAmountRounds->setText(strSettings);
}


void OverviewPage::darkSendStatus()
{
    int nBestHeight = chainActive.Tip()->nHeight;

    if(nBestHeight != darkSendPool.cachedNumBlocks)
    {
        //we we're processing lots of blocks, we'll just leave
        if(GetTime() - lastNewBlock < 10) return;
        lastNewBlock = GetTime();

        updateDarksendProgress();
    }

    if(!fEnableDarksend) {
        if(nBestHeight != darkSendPool.cachedNumBlocks)
        {
            darkSendPool.cachedNumBlocks = nBestHeight;

            ui->darksendEnabled->setText(tr("Disabled"));
            ui->darksendStatus->setText("");
            ui->toggleDarksend->setText(tr("Start Darksend Mixing"));
        }

        return;
    }

    // check darksend status and unlock if needed
    if(nBestHeight != darkSendPool.cachedNumBlocks)
    {
        // Balance and number of transactions might have changed
        darkSendPool.cachedNumBlocks = nBestHeight;

        /* *******************************************************/

        ui->darksendEnabled->setText(tr("Enabled"));
    }

    int state = darkSendPool.GetState();
    int entries = darkSendPool.GetEntriesCount();
    int accepted = darkSendPool.GetLastEntryAccepted();

    /* ** @TODO this string creation really needs some clean ups ---vertoe ** */
    QString strStatus;

    if(chainActive.Tip()->nHeight - darkSendPool.cachedLastSuccess < darkSendPool.minBlockSpacing) {
        strStatus = QString(darkSendPool.strAutoDenomResult.c_str());
    } else if(state == POOL_STATUS_IDLE) {
        strStatus = tr("Darksend is idle.");
    } else if(state == POOL_STATUS_ACCEPTING_ENTRIES) {
        if(entries == 0) {
            if(darkSendPool.strAutoDenomResult.size() == 0){
                strStatus = tr("Mixing in progress...");
            } else {
                strStatus = QString(darkSendPool.strAutoDenomResult.c_str());
            }
            showingDarkSendMessage = 0;
        } else if (accepted == 1) {
            strStatus = tr("Darksend request complete:") + " " + tr("Your transaction was accepted into the pool!");
            if(showingDarkSendMessage % 10 > 8) {
                darkSendPool.lastEntryAccepted = 0;
                showingDarkSendMessage = 0;
            }
        } else {
            if(showingDarkSendMessage % 70 <= 40) strStatus = tr("Submitted following entries to masternode: %1 / %2").arg(entries).arg(darkSendPool.GetMaxPoolTransactions());
            else if(showingDarkSendMessage % 70 <= 50) strStatus = tr("Submitted to masternode, waiting for more entries ( %1 / %2 ) %3").arg(entries).arg(darkSendPool.GetMaxPoolTransactions()).arg(" .");
            else if(showingDarkSendMessage % 70 <= 60) strStatus = tr("Submitted to masternode, waiting for more entries ( %1 / %2 ) %3").arg(entries).arg(darkSendPool.GetMaxPoolTransactions()).arg(" ..");
            else if(showingDarkSendMessage % 70 <= 70) strStatus = tr("Submitted to masternode, waiting for more entries ( %1 / %2 ) %3").arg(entries).arg(darkSendPool.GetMaxPoolTransactions()).arg(" ...");
        }
    } else if(state == POOL_STATUS_SIGNING) {
        if(showingDarkSendMessage % 70 <= 10) strStatus = tr("Found enough users, signing ...");
        else if(showingDarkSendMessage % 70 <= 20) strStatus = tr("Found enough users, signing ( waiting %1 )").arg(".");
        else if(showingDarkSendMessage % 70 <= 30) strStatus = tr("Found enough users, signing ( waiting %1 )").arg("..");
        else if(showingDarkSendMessage % 70 <= 40) strStatus = tr("Found enough users, signing ( waiting %1 )").arg("...");
    } else if(state == POOL_STATUS_TRANSMISSION) {
        strStatus = tr("Transmitting final transaction.");
    } else if (state == POOL_STATUS_IDLE) {
        strStatus = tr("Darksend is idle.");
    } else if (state == POOL_STATUS_FINALIZE_TRANSACTION) {
        strStatus = tr("Finalizing transaction.");
    } else if(state == POOL_STATUS_ERROR) {
        strStatus = tr("Darksend request incomplete:") + " " + tr(darkSendPool.lastMessage.c_str()) + " " + tr("Will retry...");
    } else if(state == POOL_STATUS_SUCCESS) {
        strStatus = tr("Darksend request complete:") + " " + tr(darkSendPool.lastMessage.c_str());
    } else if(state == POOL_STATUS_QUEUE) {
        if(showingDarkSendMessage % 70 <= 50) strStatus = tr("Submitted to masternode, waiting in queue %1").arg(".");
        else if(showingDarkSendMessage % 70 <= 60) strStatus = tr("Submitted to masternode, waiting in queue %1").arg("..");
        else if(showingDarkSendMessage % 70 <= 70) strStatus = tr("Submitted to masternode, waiting in queue %1").arg("...");
    } else {
        strStatus = tr("Unknown state: id = %1").arg(state);
    }

    if(state == POOL_STATUS_ERROR || state == POOL_STATUS_SUCCESS) darkSendPool.Check();

    QString s = tr("Last Darksend message:\n") + strStatus;

    if(s != ui->darksendStatus->text())
        LogPrintf("Last Darksend message: %s\n", strStatus.toStdString());

    ui->darksendStatus->setText(s);

    if(darkSendPool.sessionDenom == 0){
        ui->labelSubmittedDenom->setText(tr("N/A"));
    } else {
        std::string out;
        darkSendPool.GetDenominationsToString(darkSendPool.sessionDenom, out);
        QString s2(out.c_str());
        ui->labelSubmittedDenom->setText(s2);
    }

    showingDarkSendMessage++;
    darksendActionCheck++;

    // Get DarkSend Denomination Status
}

void OverviewPage::darksendAuto(){
    darkSendPool.DoAutomaticDenominating();
}

void OverviewPage::darksendReset(){
    darkSendPool.Reset();

    QMessageBox::warning(this, tr("Darksend"),
        tr("Darksend was successfully reset."),
        QMessageBox::Ok, QMessageBox::Ok);
}

void OverviewPage::toggleDarksend(){
    if(!fEnableDarksend){
        int64_t balance = pwalletMain->GetBalance();
        float minAmount = 1.49 * COIN;
        if(balance < minAmount){
            QString strMinAmount(
                BitcoinUnits::formatWithUnit(
                    walletModel->getOptionsModel()->getDisplayUnit(),
                    minAmount));
            QMessageBox::warning(this, tr("Darksend"),
                tr("Darksend requires at least %1 to use.").arg(strMinAmount),
                QMessageBox::Ok, QMessageBox::Ok);
            return;
        }

        // if wallet is locked, ask for a passphrase
        if (walletModel->getEncryptionStatus() == WalletModel::Locked)
        {
            WalletModel::UnlockContext ctx(walletModel->requestUnlock(false));
            if(!ctx.isValid())
            {
                //unlock was cancelled
                darkSendPool.cachedNumBlocks = 0;
                QMessageBox::warning(this, tr("Darksend"),
                    tr("Wallet is locked and user declined to unlock. Disabling Darksend."),
                    QMessageBox::Ok, QMessageBox::Ok);
                if (fDebug) LogPrintf("Wallet is locked and user declined to unlock. Disabling Darksend.\n");
                return;
            }
        }

    }

    darkSendPool.cachedNumBlocks = 0;
    fEnableDarksend = !fEnableDarksend;

    if(!fEnableDarksend){
        ui->toggleDarksend->setText(tr("Start Darksend Mixing"));
    } else {
        ui->toggleDarksend->setText(tr("Stop Darksend Mixing"));

        /* show darksend configuration if client has defaults set */

        if(nAnonymizeDarkcoinAmount == 0){
            DarksendConfig dlg(this);
            dlg.setModel(walletModel);
            dlg.exec();
        }

        darkSendPool.DoAutomaticDenominating();
    }
}
