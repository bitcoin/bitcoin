// Copyright (c) 2016-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_MASTERNODELIST_H
#define BITCOIN_QT_MASTERNODELIST_H

#include <primitives/transaction.h>
#include <sync.h>
#include <util/system.h>

#include <evo/types.h>

#include <QMenu>
#include <QTimer>
#include <QWidget>

#define MASTERNODELIST_UPDATE_SECONDS 3
#define MASTERNODELIST_FILTER_COOLDOWN_SECONDS 3

namespace Ui
{
class MasternodeList;
}

class ClientModel;
class WalletModel;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

namespace interfaces {
class MnEntry;
}

/** Masternode Manager page widget */
class MasternodeList : public QWidget
{
    Q_OBJECT

public:
    explicit MasternodeList(QWidget* parent = nullptr);
    ~MasternodeList();

    enum Column : uint8_t {
        SERVICE,
        TYPE,
        STATUS,
        POSE,
        REGISTERED,
        LAST_PAYMENT,
        NEXT_PAYMENT,
        PAYOUT_ADDRESS,
        OPERATOR_REWARD,
        COLLATERAL_ADDRESS,
        OWNER_ADDRESS,
        VOTING_ADDRESS,
        PROTX_HASH,
        COUNT
    };

    static int columnWidth(int column);

    void setClientModel(ClientModel* clientModel);
    void setWalletModel(WalletModel* walletModel);

private:
    QMenu* contextMenuDIP3;
    int64_t nTimeFilterUpdatedDIP3{0};
    int64_t nTimeUpdatedDIP3{0};
    bool fFilterUpdatedDIP3{true};

    QTimer* timer;
    Ui::MasternodeList* ui;
    ClientModel* clientModel{nullptr};
    WalletModel* walletModel{nullptr};

    // Protects tableWidgetMasternodesDIP3
    RecursiveMutex cs_dip3list;

    QString strCurrentFilterDIP3;

    bool mnListChanged{true};

    std::unique_ptr<const interfaces::MnEntry> GetSelectedDIP3MN();

    void updateDIP3List();

Q_SIGNALS:
    void doubleClicked(const QModelIndex&);

private Q_SLOTS:
    void showContextMenuDIP3(const QPoint&);
    void on_filterLineEditDIP3_textChanged(const QString& strFilterIn);
    void on_checkBoxMyMasternodesOnly_stateChanged(int state);

    void extraInfoDIP3_clicked();
    void copyProTxHash_clicked();
    void copyCollateralOutpoint_clicked();

    void handleMasternodeListChanged();
    void updateDIP3ListScheduled();
};
#endif // BITCOIN_QT_MASTERNODELIST_H
