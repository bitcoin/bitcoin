// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_OVERVIEWPAGE_H
#define BITCOIN_QT_OVERVIEWPAGE_H

#include <interfaces/wallet.h>
#include <qt/bitcoinunits.h>

#include <QFont>
#include <QWidget>

#include <memory>

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
    explicit OverviewPage(QWidget* parent = nullptr);
    ~OverviewPage();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);
    void showOutOfSyncWarning(bool fShow);

public Q_SLOTS:
    void coinJoinStatus(bool fForce = false);
    void setBalance(const interfaces::WalletBalances& balances);
    void setPrivacy(bool privacy);

Q_SIGNALS:
    void transactionClicked(const QModelIndex &index);
    void outOfSyncWarningClicked();

private:
    QTimer* timer{nullptr};
    Ui::OverviewPage *ui;
    ClientModel* clientModel{nullptr};
    WalletModel* walletModel{nullptr};
    interfaces::WalletBalances m_balances;
    bool m_privacy{false};
    BitcoinUnit m_display_bitcoin_unit;
    bool fShowAdvancedCJUI;
    int cachedNumISLocks{-1};

    TxViewDelegate *txdelegate;
    std::unique_ptr<TransactionFilterProxy> filter;

    void SetupTransactionList(int nNumItems);
    void DisableCoinJoinCompletely();

private Q_SLOTS:
    void toggleCoinJoin();
    void updateDisplayUnit();
    void updateCoinJoinProgress();
    void updateAdvancedCJUI(bool fShowAdvancedCJUI);
    void handleTransactionClicked(const QModelIndex &index);
    void updateAlerts(const QString &warnings);
    void updateWatchOnlyLabels(bool showWatchOnly);
    void setMonospacedFont(const QFont&);
};

#endif // BITCOIN_QT_OVERVIEWPAGE_H
