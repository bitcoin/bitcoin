// Copyright (c) 2021-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_MASTERNODEMODEL_H
#define BITCOIN_QT_MASTERNODEMODEL_H

#include <evo/dmn_types.h>

#include <QAbstractTableModel>
#include <QByteArray>
#include <QIcon>
#include <QString>

#include <memory>
#include <optional>
#include <tuple>
#include <vector>

namespace interfaces {
class MnEntry;
} // namespace interfaces

class MasternodeEntry
{
private:
    bool m_banned{false};
    int32_t m_last_paid_height{0};
    int32_t m_next_payment_height{0};
    int32_t m_pose_penalty{0};
    int32_t m_registered_height{0};
    MnType m_type{MnType::Regular};
    QByteArray m_service_key{};
    QString m_collateral_address{};
    QString m_collateral_outpoint{};
    QString m_json{};
    QString m_operator_reward{};
    QString m_owner_address{};
    QString m_payout_address{};
    QString m_protx_hash{};
    QString m_service{};
    QString m_type_description{};
    QString m_voting_address{};
    std::optional<int32_t> m_collateral_index{};
    std::optional<int32_t> m_consecutive_payments{};
    std::optional<int32_t> m_pose_ban_height{};
    std::optional<int32_t> m_pose_revived_height{};
    std::optional<QString> m_collateral_hash{};
    std::optional<QString> m_network_addresses{};
    std::optional<QString> m_platform_https_addresses{};
    std::optional<QString> m_platform_node_id{};
    std::optional<QString> m_platform_p2p_addresses{};
    std::optional<QString> m_pub_key_operator{};
    uint16_t m_operator_reward_pct{0};

public:
    explicit MasternodeEntry(const interfaces::MnEntry& dmn, const QString& collateral_address, int next_payment_height);
    ~MasternodeEntry();

    bool isBanned() const { return m_banned; }
    int lastPaidHeight() const { return m_last_paid_height; }
    int nextPaymentHeight() const { return m_next_payment_height; }
    int posePenalty() const { return m_pose_penalty; }
    int registeredHeight() const { return m_registered_height; }
    MnType type() const { return m_type; }
    std::optional<int32_t> poseBanHeight() const { return m_pose_ban_height; }
    std::optional<int32_t> poseRevivedHeight() const { return m_pose_revived_height; }
    uint16_t operatorRewardPct() const { return m_operator_reward_pct; }

    const QByteArray& serviceKey() const { return m_service_key; }
    const QString& collateralAddress() const { return m_collateral_address; }
    const QString& collateralOutpoint() const { return m_collateral_outpoint; }
    const QString& operatorReward() const { return m_operator_reward; }
    const QString& ownerAddress() const { return m_owner_address; }
    const QString& payoutAddress() const { return m_payout_address; }
    const QString& proTxHash() const { return m_protx_hash; }
    const QString& service() const { return m_service; }
    const QString& toJson() const { return m_json; }
    const QString& typeDescription() const { return m_type_description; }
    const QString& votingAddress() const { return m_voting_address; }

    auto toTie() const
    {
        return std::tie(m_banned, m_last_paid_height, m_next_payment_height, m_pose_penalty, m_service, m_operator_reward_pct);
    }
    QString toHtml() const;
};

using MasternodeEntryList = std::vector<std::unique_ptr<MasternodeEntry>>;

class MasternodeModel : public QAbstractTableModel
{
    Q_OBJECT

private:
    int m_current_height{0};
    MasternodeEntryList m_data;
    QIcon m_icon_banned;
    QIcon m_icon_enabled;

    bool isValidRow(int row) const { return row >= 0 && row < static_cast<int>(m_data.size()); }

public:
    enum Column : uint8_t {
        STATUS,
        SERVICE,
        TYPE,
        POSE,
        REGISTERED,
        LAST_PAYMENT,
        NEXT_PAYMENT,
        OPERATOR_REWARD,
        PROTX_HASH,
        COUNT
    };

    explicit MasternodeModel(QObject* parent = nullptr);
    ~MasternodeModel();

    void refreshIcons();

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void append(std::unique_ptr<MasternodeEntry>&& entry);
    void remove(int row);
    void reconcile(MasternodeEntryList&& entries);
    void setCurrentHeight(int height) { m_current_height = height; }
    const MasternodeEntry* getEntryAt(const QModelIndex& index) const;
};

#endif // BITCOIN_QT_MASTERNODEMODEL_H
