// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef OVERVIEWPAGE_H
#define OVERVIEWPAGE_H

#include <QWidget>

class ClientModel;
class TransactionFilterProxy;
class TxViewDelegate;
class WalletModel;

namespace Ui {
    class OverviewPage;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Overview ("home") page widget */
class OverviewPage : public QWidget
{
    Q_OBJECT

public:
    explicit OverviewPage(QWidget *parent = 0);
    ~OverviewPage();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);
    void showOutOfSyncWarning(bool fShow);
    void updateDarksendProgress();

public slots:
    void darkSendStatus();
    void setBalance(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance, qint64 anonymizedBalance);

signals:
    void transactionClicked(const QModelIndex &index);

private:
    QTimer *timer;
    Ui::OverviewPage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    qint64 currentBalance;
    qint64 currentUnconfirmedBalance;
    qint64 currentImmatureBalance;
    qint64 currentAnonymizedBalance;
    qint64 cachedTxLocks;
    qint64 lastNewBlock;

    int showingDarkSendMessage;
    int darksendActionCheck;
    int cachedNumBlocks;

    TxViewDelegate *txdelegate;
    TransactionFilterProxy *filter;

private slots:
    void toggleDarksend();
    void darksendAuto();
    void darksendReset();
    void updateDisplayUnit();
    void handleTransactionClicked(const QModelIndex &index);
    void updateAlerts(const QString &warnings);
};

#endif // OVERVIEWPAGE_H
