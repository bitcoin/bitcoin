// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_OVERVIEWPAGE_H
#define BITCOIN_QT_OVERVIEWPAGE_H

#include <interfaces/wallet.h>

#include <QWidget>
#include <memory>

class ClientModel;
class TransactionFilterProxy;
class TxViewDelegate;
class PlatformStyle;
class WalletModel;

namespace interfaces {
    class Node;
}

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
    explicit OverviewPage(interfaces::Node& node, const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~OverviewPage();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);
    void showOutOfSyncWarning(bool fShow);

public Q_SLOTS:
    void privateSendStatus();
    void setBalance(const interfaces::WalletBalances& balances);

Q_SIGNALS:
    void transactionClicked(const QModelIndex &index);
    void outOfSyncWarningClicked();

private:
    QTimer *timer;
    interfaces::Node& m_node;
    Ui::OverviewPage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    interfaces::WalletBalances m_balances;
    int nDisplayUnit;
    bool fShowAdvancedPSUI;

    TxViewDelegate *txdelegate;
    std::unique_ptr<TransactionFilterProxy> filter;

    void SetupTransactionList(int nNumItems);
    void DisablePrivateSendCompletely();

private Q_SLOTS:
    void togglePrivateSend();
    void privateSendAuto();
    void privateSendReset();
    void privateSendInfo();
    void updateDisplayUnit();
    void updatePrivateSendProgress();
    void updateAdvancedPSUI(bool fShowAdvancedPSUI);
    void handleTransactionClicked(const QModelIndex &index);
    void updateAlerts(const QString &warnings);
    void updateWatchOnlyLabels(bool showWatchOnly);
    void handleOutOfSyncWarningClicks();
};

#endif // BITCOIN_QT_OVERVIEWPAGE_H
