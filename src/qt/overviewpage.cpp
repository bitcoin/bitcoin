// Copyright (c) 2011-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Darkcoin developers
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

#define DECORATION_SIZE 64
#define NUM_ITEMS 3

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate(): QAbstractItemDelegate(), unit(BitcoinUnits::DRK)
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
        QString amountText = BitcoinUnits::formatWithUnit(unit, amount, true);
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
    } else {
        timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(darkSendStatus()));
        timer->start(333);
    }

    if(fMasterNode || fLiteMode){
        ui->toggleDarksend->setText("(" + tr("Disabled") + ")");
        ui->toggleDarksend->setEnabled(false);
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
    disconnect(timer, SIGNAL(timeout()), this, SLOT(darkSendStatus()));
    delete ui;
}

void OverviewPage::setBalance(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance, qint64 anonymizedBalance)
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentAnonymizedBalance = anonymizedBalance;
    ui->labelBalance->setText(BitcoinUnits::formatWithUnit(unit, balance));
    ui->labelUnconfirmed->setText(BitcoinUnits::formatWithUnit(unit, unconfirmedBalance));
    ui->labelImmature->setText(BitcoinUnits::formatWithUnit(unit, immatureBalance));
    ui->labelAnonymized->setText(BitcoinUnits::formatWithUnit(unit, anonymizedBalance));
    ui->labelTotal->setText(BitcoinUnits::formatWithUnit(unit, balance + unconfirmedBalance + immatureBalance));

    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = immatureBalance != 0;
    ui->labelImmature->setVisible(showImmature);
    ui->labelImmatureText->setVisible(showImmature);

    if(cachedTxLocks != nCompleteTXLocks){
        cachedTxLocks = nCompleteTXLocks;
        ui->listTransactions->update();
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
        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(), model->getAnonymizedBalance());
        connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64, qint64)), this, SLOT(setBalance(qint64, qint64, qint64, qint64)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        connect(ui->darksendAuto, SIGNAL(clicked()), this, SLOT(darksendAuto()));
        connect(ui->darksendReset, SIGNAL(clicked()), this, SLOT(darksendReset()));
        connect(ui->toggleDarksend, SIGNAL(clicked()), this, SLOT(toggleDarksend()));
    }

    // update the display unit, to not use the default ("DRK")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        if(currentBalance != -1)
            setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance, currentAnonymizedBalance);

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
        return;
    }

    //get denominated unconfirmed inputs
    if(pwalletMain->GetDenominatedBalance(true, true) > 0)
    {
        QString s(tr("Found unconfirmed denominated outputs, will wait till they confirm to recalculate."));
        ui->darksendProgress->setToolTip(s);
        return;
    }

    //Get the anon threshold
    int64_t nMaxToAnonymize = nAnonymizeDarkcoinAmount*COIN;

    // If it's more than the wallet amount, limit to that.
    if(nMaxToAnonymize > nBalance) nMaxToAnonymize = nBalance;

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
    if(progress > 100) progress = 100;

    ui->darksendProgress->setValue(progress);

    std::ostringstream convert;
    convert << "Progress: " << progress << "%, inputs have an average of " << pwalletMain->GetAverageAnonymizedRounds() << " of " << nDarksendRounds << " rounds";
    QString s(convert.str().c_str());
    ui->darksendProgress->setToolTip(s);
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

        QString strSettings(" " + tr("Rounds"));
        strSettings.prepend(QString::number(nDarksendRounds)).prepend(" / ");
        strSettings.prepend(BitcoinUnits::formatWithUnit(
            walletModel->getOptionsModel()->getDisplayUnit(),
            nAnonymizeDarkcoinAmount * COIN)
        );

        ui->labelAmountRounds->setText(strSettings);
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
    std::ostringstream convert;

    if(state == POOL_STATUS_IDLE) {
        convert << tr("Darksend is idle.").toStdString();
    } else if(state == POOL_STATUS_ACCEPTING_ENTRIES) {
        if(entries == 0) {
            if(darkSendPool.strAutoDenomResult.size() == 0){
                convert << tr("Mixing in progress...").toStdString();
            } else {
                convert << darkSendPool.strAutoDenomResult;
            }
            showingDarkSendMessage = 0;
        } else if (accepted == 1) {
            convert << tr("Darksend request complete: Your transaction was accepted into the pool!").toStdString();
            if(showingDarkSendMessage % 10 > 8) {
                darkSendPool.lastEntryAccepted = 0;
                showingDarkSendMessage = 0;
            }
        } else {
            if(showingDarkSendMessage % 70 <= 40) convert << tr("Submitted following entries to masternode:").toStdString() << " " << entries << "/" << darkSendPool.GetMaxPoolTransactions();
            else if(showingDarkSendMessage % 70 <= 50) convert << tr("Submitted to masternode, Waiting for more entries").toStdString() << " (" << entries << "/" << darkSendPool.GetMaxPoolTransactions() << " ) .";
            else if(showingDarkSendMessage % 70 <= 60) convert << tr("Submitted to masternode, Waiting for more entries").toStdString() << " (" << entries << "/" << darkSendPool.GetMaxPoolTransactions() << " ) ..";
            else if(showingDarkSendMessage % 70 <= 70) convert << tr("Submitted to masternode, Waiting for more entries").toStdString() << " (" << entries << "/" << darkSendPool.GetMaxPoolTransactions() << " ) ...";
        }
    } else if(state == POOL_STATUS_SIGNING) {
        if(showingDarkSendMessage % 70 <= 10) convert << tr("Found enough users, signing ...").toStdString();
        else if(showingDarkSendMessage % 70 <= 20) convert << tr("Found enough users, signing ( waiting. )").toStdString();
        else if(showingDarkSendMessage % 70 <= 30) convert << tr("Found enough users, signing ( waiting.. )").toStdString();
        else if(showingDarkSendMessage % 70 <= 40) convert << tr("Found enough users, signing ( waiting... )").toStdString();
    } else if(state == POOL_STATUS_TRANSMISSION) {
        convert << tr("Transmitting final transaction.").toStdString();
    } else if (state == POOL_STATUS_IDLE) {
        convert << tr("Darksend is idle.").toStdString();
    } else if (state == POOL_STATUS_FINALIZE_TRANSACTION) {
        convert << tr("Finalizing transaction.").toStdString();
    } else if(state == POOL_STATUS_ERROR) {
        convert << tr("Darksend request incomplete:").toStdString() << " " << darkSendPool.lastMessage << ". " << tr("Will retry...").toStdString();
    } else if(state == POOL_STATUS_SUCCESS) {
        convert << tr("Darksend request complete:").toStdString() << " " << darkSendPool.lastMessage;
    } else if(state == POOL_STATUS_QUEUE) {
        if(showingDarkSendMessage % 70 <= 50) convert << tr("Submitted to masternode, waiting in queue .").toStdString();
        else if(showingDarkSendMessage % 70 <= 60) convert << tr("Submitted to masternode, waiting in queue ..").toStdString();
        else if(showingDarkSendMessage % 70 <= 70) convert << tr("Submitted to masternode, waiting in queue ...").toStdString();
    } else {
        convert << tr("Unknown state:").toStdString() << " id = " << state;
    }

    if(state == POOL_STATUS_ERROR || state == POOL_STATUS_SUCCESS) darkSendPool.Check();

    QString s(convert.str().c_str());
    s = tr("Last Darksend message:\n") + s;

    if(s != ui->darksendStatus->text())
        LogPrintf("Last Darksend message: %s\n", convert.str().c_str());

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
