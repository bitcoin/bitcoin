// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef OVERVIEWPAGE_H
#define OVERVIEWPAGE_H

#include <QWidget>

class ClientModel;
class Credits_TransactionFilterProxy;
class Bitcredit_TxViewDelegate;
class Bitcredit_WalletModel;

namespace Ui {
    class Credits_OverviewPage;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Overview ("home") page widget */
class Credits_OverviewPage : public QWidget
{
    Q_OBJECT

public:
    explicit Credits_OverviewPage(QWidget *parent = 0);
    ~Credits_OverviewPage();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(Bitcredit_WalletModel *bitcredit_model, Bitcredit_WalletModel *deposit_model);
    void showOutOfSyncWarning(bool fShow);

public slots:
    void setBalance(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance, qint64 preparedDepositBalance, qint64 inDepositBalance);
    void refreshBalance();

signals:
    void transactionClicked(const QModelIndex &index);

private:
    Ui::Credits_OverviewPage *ui;
    ClientModel *clientModel;
    Bitcredit_WalletModel *bitcredit_model;
    Bitcredit_WalletModel *deposit_model;
    qint64 currentBalance;
    qint64 currentUnconfirmedBalance;
    qint64 currentImmatureBalance;
    qint64 currentPreparedDepositBalance;
    qint64 currentInDepositBalance;

    Bitcredit_TxViewDelegate *txdelegate;
    Credits_TransactionFilterProxy *filter;

private slots:
    void updateDisplayUnit();
    void handleTransactionClicked(const QModelIndex &index);
    void updateAlerts(const QString &warnings);
};

#endif // OVERVIEWPAGE_H
