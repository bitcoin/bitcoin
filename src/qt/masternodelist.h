// Copyright (c) 2016-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_MASTERNODELIST_H
#define BITCOIN_QT_MASTERNODELIST_H

#include <util/system.h>

#include <QMenu>
#include <QSortFilterProxyModel>
#include <QString>
#include <QTimer>
#include <QWidget>

#include <set>

namespace Ui {
class MasternodeList;
} // namespace Ui

class ClientModel;
class MasternodeEntry;
class MasternodeModel;
class WalletModel;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

class MasternodeListSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit MasternodeListSortFilterProxyModel(QObject* parent = nullptr) :
        QSortFilterProxyModel(parent) {}

    void setShowMyMasternodesOnly(bool show) { m_show_my_only = show; }
    void setMyMasternodeHashes(std::set<QString> hashes) { m_my_mn_hashes = std::move(hashes); }
    void forceInvalidateFilter() { invalidateFilter(); }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

private:
    bool m_show_my_only{false};
    std::set<QString> m_my_mn_hashes;
};

/** Masternode Manager page widget */
class MasternodeList : public QWidget
{
    Q_OBJECT

public:
    explicit MasternodeList(QWidget* parent = nullptr);
    ~MasternodeList();

    void setClientModel(ClientModel* clientModel);
    void setWalletModel(WalletModel* walletModel);

private:
    QMenu* contextMenuDIP3;
    int64_t nTimeUpdatedDIP3{0};

    QTimer* timer;
    Ui::MasternodeList* ui;
    ClientModel* clientModel{nullptr};
    WalletModel* walletModel{nullptr};

    MasternodeModel* m_model{nullptr};
    MasternodeListSortFilterProxyModel* m_proxy_model{nullptr};

    bool mnListChanged{true};

    const MasternodeEntry* GetSelectedEntry();

    void updateDIP3List();
    void updateMyMasternodeHashes();

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
    void updateFilteredCount();
};

#endif // BITCOIN_QT_MASTERNODELIST_H
