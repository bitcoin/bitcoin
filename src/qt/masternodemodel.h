// Copyright (c) 2021-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_MASTERNODEMODEL_H
#define BITCOIN_QT_MASTERNODEMODEL_H

#include <evo/dmn_types.h>

#include <QByteArray>
#include <QString>

#include <memory>
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
    uint16_t operatorRewardPct() const { return m_operator_reward_pct; }
};

#endif // BITCOIN_QT_MASTERNODEMODEL_H
