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

#include <atomic>
#include <memory>
#include <set>

namespace interfaces {
class MnList;
using MnListPtr = std::shared_ptr<MnList>;
} // namespace interfaces
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
    enum class TypeFilter : uint8_t {
        All,
        Regular,
        Evo,
        COUNT
    };

    explicit MasternodeListSortFilterProxyModel(QObject* parent = nullptr) :
        QSortFilterProxyModel(parent) {}

    void forceInvalidateFilter() { invalidateFilter(); }
    void setHideBanned(bool hide) { m_hide_banned = hide; }
    void setMyMasternodeHashes(std::set<QString> hashes) { m_my_mn_hashes = std::move(hashes); }
    void setShowOwnedOnly(bool show) { m_show_owned_only = show; }
    void setTypeFilter(TypeFilter type) { m_type_filter = type; }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

private:
    bool m_hide_banned{true};
    bool m_show_owned_only{false};
    std::set<QString> m_my_mn_hashes;
    TypeFilter m_type_filter{TypeFilter::All};
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

protected:
    void changeEvent(QEvent* event) override;

private:
    QMenu* contextMenuDIP3;
    int64_t nTimeUpdatedDIP3{0};

    QTimer* timer;
    Ui::MasternodeList* ui;
    ClientModel* clientModel{nullptr};
    WalletModel* walletModel{nullptr};

    MasternodeModel* m_model{nullptr};
    MasternodeListSortFilterProxyModel* m_proxy_model{nullptr};

    std::atomic<bool> m_mn_list_changed{true};

    const MasternodeEntry* GetSelectedEntry();

    bool updateDIP3List();
    void updateMyMasternodeHashes(const interfaces::MnListPtr& mnList);

Q_SIGNALS:
    void doubleClicked(const QModelIndex&);

private Q_SLOTS:
    void copyCollateralOutpoint_clicked();
    void copyProTxHash_clicked();
    void extraInfoDIP3_clicked();
    void filterByCollateralAddress();
    void filterByPayoutAddress();
    void filterByOwnerAddress();
    void filterByVotingAddress();
    void handleMasternodeListChanged();
    void on_checkBoxHideBanned_stateChanged(int state);
    void on_checkBoxOwned_stateChanged(int state);
    void on_comboBoxType_currentIndexChanged(int index);
    void on_filterText_textChanged(const QString& strFilterIn);
    void showContextMenuDIP3(const QPoint&);
    void updateDIP3ListScheduled();
    void updateFilteredCount();
};

#endif // BITCOIN_QT_MASTERNODELIST_H
