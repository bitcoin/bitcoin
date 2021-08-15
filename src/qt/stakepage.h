// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_STAKEPAGE_H
#define BITCOIN_QT_STAKEPAGE_H

#include <interfaces/wallet.h>
#include <qt/sendcoinsdialog.h>

#include <QWidget>
#include <memory>

class ClientModel;
class TransactionFilterProxy;
class PlatformStyle;
class WalletModel;
class TransactionView;

namespace Ui {
    class StakePage;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Stake page widget */
class StakePage : public QWidget
{
    Q_OBJECT

public:
    explicit StakePage(const PlatformStyle *platformStyle, QWidget *parent = nullptr);
    ~StakePage();

    void setClientModel(ClientModel *clientModel);
    void setWalletModel(WalletModel *walletModel);

public Q_SLOTS:
    void setBalance(const interfaces::WalletBalances& balances);
    void numBlocksChanged(int count, const QDateTime& blockDate, double nVerificationProgress, bool headers);
    void updateEncryptionStatus();

Q_SIGNALS:
    void requireUnlock(bool fromMenu);
    void coinsSent(const uint256& txid);
    void message(const QString &title, const QString &message, unsigned int style);

private:
    Ui::StakePage *ui;
    ClientModel *clientModel;
    WalletModel *walletModel;
    const PlatformStyle* const platformStyle;
    TransactionView* transactionView;
    TransactionView* receivedDelegationsView;
    TransactionView* myDelegationsView;
    SendCoinsDialog* sendCoinsDialog;
    interfaces::WalletBalances m_balances;
    int64_t m_subsidy;
    uint64_t m_networkWeight;
    double m_expectedAnnualROI;

private Q_SLOTS:
    void updateDisplayUnit();
    void on_checkStake_clicked(bool checked);
    void on_newAddressButton_clicked();
    void coldStakerControlFeaturesChanged(bool);

private:
    void updateSubsidy();
    void updateNetworkWeight();
    void updateAnnualROI();
};

#endif // BITCOIN_QT_STAKEPAGE_H
