// Copyright (c) 2021-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/masternodemodel.h>

#include <chainparams.h>
#include <interfaces/node.h>
#include <key_io.h>
#include <pubkey.h>
#include <script/script.h>
#include <script/standard.h>

#include <qt/clientmodel.h>
#include <qt/guiutil.h>

#include <QHash>
#include <QObject>
#include <QStringList>

#include <univalue.h>

#include <set>

namespace {
constexpr int64_t DAY_SECS{24 * 60 * 60};

std::optional<QString> JoinArray(const UniValue& arr)
{
    if (!arr.isArray() || arr.empty()) return std::nullopt;
    QStringList list;
    for (size_t i = 0; i < arr.size(); ++i) {
        if (arr[i].isStr()) {
            list << QString::fromStdString(arr[i].get_str());
        }
    }
    return list.isEmpty() ? std::nullopt : std::optional<QString>(list.join(", "));
}
} // anonymous namespace

MasternodeEntry::MasternodeEntry(const interfaces::MnEntry& dmn, const QString& collateral_address, int next_payment_height) :
    m_banned{dmn.isBanned()},
    m_last_paid_height{dmn.getLastPaidHeight()},
    m_next_payment_height{next_payment_height},
    m_pose_penalty{dmn.getPoSePenalty()},
    m_registered_height{dmn.getRegisteredHeight()},
    m_type{dmn.getType()},
    m_collateral_address{collateral_address},
    m_collateral_outpoint{QString::fromStdString(dmn.getCollateralOutpoint().ToStringShort())},
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

    const auto json{dmn.toJson()};
    m_json = QString::fromStdString(json.write(2));
    if (const auto& val = json.find_value("collateralHash"); val.isStr()) {
        m_collateral_hash = QString::fromStdString(val.get_str());
    }
    if (const auto& val = json.find_value("collateralIndex"); val.isNum()) {
        m_collateral_index = val.getInt<int32_t>();
    }
    if (const auto& state = json.find_value("state"); state.isObject()) {
        if (const auto& val = state.find_value("consecutivePayments"); val.isNum()) {
            m_consecutive_payments = val.getInt<int32_t>();
        }
        if (const auto& val = state.find_value("PoSeBanHeight"); val.isNum()) {
            m_pose_ban_height = val.getInt<int32_t>();
        }
        if (const auto& val = state.find_value("PoSeRevivedHeight"); val.isNum()) {
            m_pose_revived_height = val.getInt<int32_t>();
        }
        if (const auto& val = state.find_value("pubKeyOperator"); val.isStr()) {
            m_pub_key_operator = QString::fromStdString(val.get_str());
        }
        if (const auto& val = state.find_value("addresses"); val.isObject()) {
            m_network_addresses = JoinArray(val.find_value("core_p2p"));
            m_platform_p2p_addresses = JoinArray(val.find_value("platform_p2p"));
            m_platform_https_addresses = JoinArray(val.find_value("platform_https"));
        }
        if (const auto& val = state.find_value("platformNodeID"); val.isStr()) {
            m_platform_node_id = QString::fromStdString(val.get_str());
        }
    }
}

MasternodeEntry::~MasternodeEntry() = default;

QString MasternodeEntry::toHtml() const
{
    QString ret;
    ret.reserve(4000);
    ret += "<html>";

    ret += "<b>" + QObject::tr("ProTx Hash") + ":</b> " + m_protx_hash.toHtmlEscaped() + "<br>";
    if (m_pub_key_operator) {
        ret += "<b>" + QObject::tr("Public Key Operator") + ":</b> " + m_pub_key_operator->toHtmlEscaped() + "<br>";
    }
    ret += "<b>" + QObject::tr("Owner Address") + ":</b> " + m_owner_address.toHtmlEscaped() + "<br>";
    ret += "<b>" + QObject::tr("Payout Address") + ":</b> " + m_payout_address.toHtmlEscaped() + "<br>";
    ret += "<b>" + QObject::tr("Voting Address") + ":</b> " + m_voting_address.toHtmlEscaped() + "<br>";
    ret += "<b>" + QObject::tr("Collateral Address") + ":</b> " + m_collateral_address.toHtmlEscaped() + "<br>";
    if (m_collateral_hash) {
        ret += "<b>" + QObject::tr("Collateral Hash") + ":</b> " + m_collateral_hash->toHtmlEscaped() + "<br>";
    }
    if (m_collateral_index) {
        ret += "<b>" + QObject::tr("Collateral Index") + ":</b> " + QString::number(*m_collateral_index) + "<br>";
    }
    ret += "<br>";

    ret += "<b>" + QObject::tr("Masternode Type") + ":</b> " + m_type_description.toHtmlEscaped() + "<br>";
    ret += "<b>" + QObject::tr("Registered Height") + ":</b> " + QString::number(m_registered_height) + "<br>";
    ret += "<b>" + QObject::tr("Last Paid Height") + ":</b> " + QString::number(m_last_paid_height) + "<br>";
    if (m_consecutive_payments) {
        ret += "<b>" + QObject::tr("Consecutive Payments") + ":</b> " + QString::number(*m_consecutive_payments) + "<br>";
    }
    ret += "<b>" + QObject::tr("Operator Reward") + ":</b> " + QString::number(m_operator_reward_pct / 100.0, 'f', 2) + "%<br>";
    if (m_network_addresses) {
        ret += "<b>" + QObject::tr("Network Addresses") + ":</b> " + m_network_addresses->toHtmlEscaped() + "<br>";
    }

    if (m_platform_https_addresses) {
        ret += "<b>" + QObject::tr("Platform HTTPS Addresses") + ":</b> " + m_platform_https_addresses->toHtmlEscaped() + "<br>";
    }
    if (m_platform_p2p_addresses) {
        ret += "<b>" + QObject::tr("Platform P2P Addresses") + ":</b> " + m_platform_p2p_addresses->toHtmlEscaped() + "<br>";
    }
    if (m_platform_node_id) {
        ret += "<b>" + QObject::tr("Platform Node ID") + ":</b> " + m_platform_node_id->toHtmlEscaped() + "<br>";
    }
    ret += "<br>";

    ret += "<b>" + QObject::tr("PoSe Penalty") + ":</b> " + QString::number(m_pose_penalty) + "<br>";
    if (m_pose_ban_height && *m_pose_ban_height != -1) {
        ret += "<b>" + QObject::tr("PoSe Ban Height") + ":</b> " + QString::number(*m_pose_ban_height) + "<br>";
    }
    if (m_pose_revived_height && *m_pose_revived_height != -1) {
        ret += "<b>" + QObject::tr("PoSe Revived Height") + ":</b> " + QString::number(*m_pose_revived_height) + "<br>";
    }

    ret += "</html>";
    return ret;
}

MasternodeModel::MasternodeModel(QObject* parent) :
    QAbstractTableModel(parent)
{
    refreshIcons();
}

void MasternodeModel::refreshIcons()
{
    m_icon_banned = GUIUtil::getIcon("warning", GUIUtil::ThemedColor::RED);
    m_icon_enabled = GUIUtil::getIcon("synced", GUIUtil::ThemedColor::GREEN);
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

    if (role != Qt::DecorationRole && role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::UserRole && role != Qt::ToolTipRole) {
        return {};
    }

    const auto* entry = m_data[index.row()].get();
    if (!entry) {
        return {};
    }

    if (role == Qt::ToolTipRole) {
        if (index.column() == Column::STATUS && m_current_height > 0) {
            const int32_t blocks_per_day = DAY_SECS / Params().GetConsensus().nPowTargetSpacing;
            if (entry->isBanned()) {
                if (auto ban_height = entry->poseBanHeight(); ban_height && *ban_height > 0) {
                    const auto days{(m_current_height - *ban_height) / blocks_per_day};
                    return days > 0 ? tr("Banned for %n day(s)", "", days) : tr("Banned for less than a day");
                } else {
                    return tr("Banned");
                }
            } else {
                int32_t active_height = entry->registeredHeight();
                if (auto revived_height = entry->poseRevivedHeight(); revived_height && *revived_height > 0) {
                    active_height = *revived_height;
                }
                const auto days{(m_current_height - active_height) / blocks_per_day};
                return days > 0 ? tr("Active for %n day(s)", "", days) : tr("Active for less than a day");
            }
        }
        return {};
    } else if (role == Qt::DecorationRole) {
        if (index.column() == Column::STATUS) {
            return entry->isBanned() ? m_icon_banned : m_icon_enabled;
        }
        return {};
    } else if (role == Qt::UserRole) {
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
            return {};
        case Column::POSE:
            return QString::number(entry->posePenalty());
        case Column::REGISTERED:
            return QString::number(entry->registeredHeight());
        case Column::LAST_PAYMENT:
            return QString::number(entry->lastPaidHeight());
        case Column::NEXT_PAYMENT:
            return entry->nextPaymentHeight() > 0 ? QString::number(entry->nextPaymentHeight()) : tr("UNKNOWN");
        case Column::OPERATOR_REWARD:
            return entry->operatorReward();
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
        case Column::OPERATOR_REWARD:
            return entry->operatorRewardPct();
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
        return {};
    case Column::POSE:
        return tr("PoSe Score");
    case Column::REGISTERED:
        return tr("Registered");
    case Column::LAST_PAYMENT:
        return tr("Last Paid");
    case Column::NEXT_PAYMENT:
        return tr("Next Payment");
    case Column::OPERATOR_REWARD:
        return tr("Operator Reward");
    case Column::PROTX_HASH:
        return tr("ProTx Hash");
    default:
        return {};
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
    QHash<QString, int> existing;
    for (int i = 0; i < static_cast<int>(m_data.size()); ++i) {
        existing[m_data[i]->proTxHash()] = i;
    }

    std::set<int> present;
    std::vector<int> changed;
    std::vector<std::unique_ptr<MasternodeEntry>> add;
    for (auto& entry : entries) {
        if (auto it = existing.find(entry->proTxHash()); it != existing.end()) {
            int idx = it.value();
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
