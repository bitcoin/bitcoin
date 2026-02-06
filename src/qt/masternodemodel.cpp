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
