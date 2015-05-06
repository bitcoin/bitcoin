// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_OVERVIEWPAGE_H
#define BITCOIN_OVERVIEWPAGE_H

#include <QWidget>

class ClientModel;
class Bitcoin_TransactionFilterProxy;
class Bitcoin_TxViewDelegate;
class Bitcoin_WalletModel;

namespace Ui {
    class Bitcoin_OverviewPage;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Overview ("home") page widget */
class Bitcoin_OverviewPage : public QWidget
{
    Q_OBJECT

public:
    explicit Bitcoin_OverviewPage(QWidget *parent = 0);
    ~Bitcoin_OverviewPage();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(Bitcoin_WalletModel *walletModel);
    void showOutOfSyncWarning(bool fShow);

public slots:
    void setBalance(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance);

signals:
    void transactionClicked(const QModelIndex &index);

private:
    Ui::Bitcoin_OverviewPage *ui;
    ClientModel *clientModel;
    Bitcoin_WalletModel *walletModel;
    qint64 currentBalance;
    qint64 currentUnconfirmedBalance;
    qint64 currentImmatureBalance;

    Bitcoin_TxViewDelegate *txdelegate;
    Bitcoin_TransactionFilterProxy *filter;

private slots:
    void updateDisplayUnit();
    void handleTransactionClicked(const QModelIndex &index);
    void updateAlerts(const QString &warnings);
};

#endif // BITCOIN_OVERVIEWPAGE_H
