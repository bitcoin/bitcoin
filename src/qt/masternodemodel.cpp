// Copyright (c) 2021-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/masternodemodel.h>

#include <interfaces/node.h>
#include <key_io.h>
#include <pubkey.h>
#include <script/script.h>
#include <script/standard.h>

#include <QObject>

#include <univalue.h>

#include <map>
#include <set>

MasternodeEntry::MasternodeEntry(const interfaces::MnEntry& dmn, const QString& collateral_address, int next_payment_height) :
    m_banned{dmn.isBanned()},
    m_last_paid_height{dmn.getLastPaidHeight()},
    m_next_payment_height{next_payment_height},
    m_pose_penalty{dmn.getPoSePenalty()},
    m_registered_height{dmn.getRegisteredHeight()},
    m_type{dmn.getType()},
    m_collateral_address{collateral_address},
    m_collateral_outpoint{QString::fromStdString(dmn.getCollateralOutpoint().ToStringShort())},
    m_json{QString::fromStdString(dmn.toJson().write(2))},
    m_owner_address{QString::fromStdString(EncodeDestination(PKHash(dmn.getKeyIdOwner())))},
    m_protx_hash{QString::fromStdString(dmn.getProTxHash().ToString())},
    m_service{QString::fromStdString(dmn.getNetInfoPrimary().ToStringAddrPort())},
    m_type_description{QString::fromStdString(std::string(GetMnType(dmn.getType()).description))},
    m_voting_address{QString::fromStdString(EncodeDestination(PKHash(dmn.getKeyIdVoting())))},
    m_operator_reward_pct{dmn.getOperatorReward()}
{
    auto addr_key = dmn.getNetInfoPrimary().GetKey();
    m_service_key = QByteArray(reinterpret_cast<const char*>(addr_key.data()), addr_key.size());

    if (CTxDestination dest; ExtractDestination(dmn.getScriptPayout(), dest)) {
        m_payout_address = QString::fromStdString(EncodeDestination(dest));
    } else {
        m_payout_address = QObject::tr("UNKNOWN");
    }

    if (m_operator_reward_pct) {
        m_operator_reward = QString::number(m_operator_reward_pct / 100.0, 'f', 2) + "%";
        if (dmn.getScriptOperatorPayout() != CScript()) {
            CTxDestination operatorDest;
            if (ExtractDestination(dmn.getScriptOperatorPayout(), operatorDest)) {
                m_operator_reward += " " + QObject::tr("to %1").arg(QString::fromStdString(EncodeDestination(operatorDest)));
            } else {
                m_operator_reward += " " + QObject::tr("to UNKNOWN");
            }
        } else {
            m_operator_reward += " " + QObject::tr("but not claimed");
        }
    } else {
        m_operator_reward = QObject::tr("NONE");
    }
}

MasternodeEntry::~MasternodeEntry() = default;

MasternodeModel::MasternodeModel(QObject* parent) :
    QAbstractTableModel(parent)
{
}

MasternodeModel::~MasternodeModel() = default;

int MasternodeModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_data.size());
}

int MasternodeModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return Column::COUNT;
}

QVariant MasternodeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || !isValidRow(index.row())) {
        return {};
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::UserRole) {
        return {};
    }

    const auto* entry = m_data[index.row()].get();
    if (!entry) {
        return {};
    }

    if (role == Qt::UserRole) {
        return QString{
            entry->service() + " " +
            entry->typeDescription() + " " +
            QString::number(entry->posePenalty()) + " " +
            QString::number(entry->registeredHeight()) + " " +
            QString::number(entry->lastPaidHeight()) + " " +
            (entry->nextPaymentHeight() > 0 ? QString::number(entry->nextPaymentHeight()) : "") + " " +
            entry->payoutAddress() + " " +
            entry->operatorReward() + " " +
            entry->collateralAddress() + " " +
            entry->ownerAddress() + " " +
            entry->votingAddress() + " " +
            entry->proTxHash()
        };
    } else if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case Column::SERVICE:
            return entry->service();
        case Column::TYPE:
            return entry->typeDescription();
        case Column::STATUS:
            return entry->isBanned() ? tr("POSE_BANNED") : tr("ENABLED");
        case Column::POSE:
            return QString::number(entry->posePenalty());
        case Column::REGISTERED:
            return QString::number(entry->registeredHeight());
        case Column::LAST_PAYMENT:
            return QString::number(entry->lastPaidHeight());
        case Column::NEXT_PAYMENT:
            return entry->nextPaymentHeight() > 0 ? QString::number(entry->nextPaymentHeight()) : tr("UNKNOWN");
        case Column::PAYOUT_ADDRESS:
            return entry->payoutAddress();
        case Column::OPERATOR_REWARD:
            return entry->operatorReward();
        case Column::COLLATERAL_ADDRESS:
            return entry->collateralAddress();
        case Column::OWNER_ADDRESS:
            return entry->ownerAddress();
        case Column::VOTING_ADDRESS:
            return entry->votingAddress();
        case Column::PROTX_HASH:
            return entry->proTxHash();
        default:
            return {};
        }
    } else if (role == Qt::EditRole) {
        // Edit role is used for sorting, return raw values where possible
        switch (index.column()) {
        case Column::SERVICE:
            return entry->serviceKey();
        case Column::TYPE:
            return static_cast<int>(entry->type());
        case Column::STATUS:
            return entry->isBanned() ? 1 : 0;
        case Column::POSE:
            return entry->posePenalty();
        case Column::REGISTERED:
            return entry->registeredHeight();
        case Column::LAST_PAYMENT:
            return entry->lastPaidHeight();
        case Column::NEXT_PAYMENT:
            return entry->nextPaymentHeight();
        case Column::PAYOUT_ADDRESS:
            return entry->payoutAddress();
        case Column::OPERATOR_REWARD:
            return entry->operatorRewardPct();
        case Column::COLLATERAL_ADDRESS:
            return entry->collateralAddress();
        case Column::OWNER_ADDRESS:
            return entry->ownerAddress();
        case Column::VOTING_ADDRESS:
            return entry->votingAddress();
        case Column::PROTX_HASH:
            return entry->proTxHash();
        default:
            return {};
        }
    }
    return {};
}

QVariant MasternodeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }

    switch (section) {
    case Column::SERVICE:
        return tr("Service");
    case Column::TYPE:
        return tr("Type");
    case Column::STATUS:
        return tr("Status");
    case Column::POSE:
        return tr("PoSe Score");
    case Column::REGISTERED:
        return tr("Registered");
    case Column::LAST_PAYMENT:
        return tr("Last Paid");
    case Column::NEXT_PAYMENT:
        return tr("Next Payment");
    case Column::PAYOUT_ADDRESS:
        return tr("Payout Address");
    case Column::OPERATOR_REWARD:
        return tr("Operator Reward");
    case Column::COLLATERAL_ADDRESS:
        return tr("Collateral Address");
    case Column::OWNER_ADDRESS:
        return tr("Owner Address");
    case Column::VOTING_ADDRESS:
        return tr("Voting Address");
    case Column::PROTX_HASH:
        return tr("ProTx Hash");
    default:
        return {};
    }
}

int MasternodeModel::columnWidth(int section)
{
    switch (section) {
    case Column::SERVICE:
        return 200;
    case Column::TYPE:
        return 160;
    case Column::STATUS:
    case Column::POSE:
    case Column::REGISTERED:
    case Column::LAST_PAYMENT:
        return 80;
    case Column::NEXT_PAYMENT:
        return 100;
    case Column::PAYOUT_ADDRESS:
    case Column::OPERATOR_REWARD:
    case Column::COLLATERAL_ADDRESS:
    case Column::OWNER_ADDRESS:
    case Column::VOTING_ADDRESS:
        return 130;
    case Column::PROTX_HASH:
    default:
        return 80;
    }
}

void MasternodeModel::append(std::unique_ptr<MasternodeEntry>&& entry)
{
    beginInsertRows({}, rowCount(), rowCount());
    m_data.push_back(std::move(entry));
    endInsertRows();
}

void MasternodeModel::remove(int row)
{
    if (!isValidRow(row)) {
        return;
    }
    beginRemoveRows({}, row, row);
    m_data.erase(m_data.begin() + row);
    endRemoveRows();
}

void MasternodeModel::reconcile(MasternodeEntryList&& entries)
{
    std::map<QString, int> existing;
    for (int i = 0; i < static_cast<int>(m_data.size()); ++i) {
        existing[m_data[i]->proTxHash()] = i;
    }

    std::set<int> present;
    std::vector<int> changed;
    std::vector<std::unique_ptr<MasternodeEntry>> add;
    for (auto& entry : entries) {
        if (auto it = existing.find(entry->proTxHash()); it != existing.end()) {
            int idx = it->second;
            present.insert(idx);
            if (m_data[idx]->toTie() != entry->toTie()) {
                m_data[idx] = std::move(entry);
                changed.push_back(idx);
            }
        } else {
            // New entry
            add.push_back(std::move(entry));
        }
    }

    // Remove entries no longer present
    std::vector<int> removed;
    for (int i = static_cast<int>(m_data.size()) - 1; i >= 0; --i) {
        if (present.find(i) == present.end()) {
            removed.push_back(i);
            beginRemoveRows({}, i, i);
            m_data.erase(m_data.begin() + i);
            endRemoveRows();
        }
    }

    // Emit dataChanged with adjusted indices
    for (int idx : changed) {
        int offset{0};
        for (int removed_idx : removed) {
            if (removed_idx < idx) {
                ++offset;
            }
        }
        int jdx = idx - offset;
        Q_EMIT dataChanged(index(jdx, 0), index(jdx, Column::COUNT - 1));
    }

    // Append new entries
    if (!add.empty()) {
        int first = static_cast<int>(m_data.size());
        int last = first + static_cast<int>(add.size()) - 1;
        beginInsertRows({}, first, last);
        for (auto& entry : add) {
            m_data.push_back(std::move(entry));
        }
        endInsertRows();
    }
}

const MasternodeEntry* MasternodeModel::getEntryAt(const QModelIndex& index) const
{
    if (!index.isValid() || !isValidRow(index.row())) {
        return nullptr;
    }
    return m_data[index.row()].get();
}
