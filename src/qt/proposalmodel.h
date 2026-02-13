// Copyright (c) 2021-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PROPOSALMODEL_H
#define BITCOIN_QT_PROPOSALMODEL_H

#include <interfaces/node.h>
#include <governance/object.h>
#include <saltedhasher.h>
#include <uint256.h>

#include <qt/bitcoinunits.h>

#include <QAbstractTableModel>
#include <QDateTime>
#include <QIcon>
#include <QString>

#include <array>
#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

class ClientModel;

enum class ProposalStatus : uint8_t {
    Confirming,
    Failing,
    Funded,
    Lapsed,
    Passing,
    Pending,
    Unfunded,
    Voting,
};

class Proposal
{
private:
    ClientModel* clientModel;
    bool m_is_broadcast{true};
    int m_block_height{0};
    int m_collateral_confs{0};
    interfaces::GOV::GovernanceInfo m_gov_info;
    const CGovernanceObject govObj;

    CAmount m_paymentAmount{0};
    interfaces::GOV::Votes m_votes;
    QDateTime m_date_collateral{};
    QDateTime m_endDate{};
    QDateTime m_startDate{};
    QString m_address{};
    QString m_hash_collateral{};
    QString m_hash_object{};
    QString m_hash_parent{};
    QString m_title{};
    QString m_url{};
    std::optional<int> m_funded_height{};
    uint256 m_objHash{};

public:
    explicit Proposal(ClientModel* _clientModel, const CGovernanceObject& _govObj,
                      const interfaces::GOV::GovernanceInfo& govInfo, int collateral_confs,
                      bool is_broadcast);

    bool isActive() const;
    bool isBroadcast() const { return m_is_broadcast; }
    CAmount paymentAmount() const { return m_paymentAmount; }
    const uint256& objHash() const { return m_objHash; }
    int blocksUntilSuperblock() const;
    int collateralConfs() const;
    int paymentsRequested() const;
    int requiredConfs() const;
    int32_t getAbsoluteYesCount() const { return m_votes.m_yes - m_votes.m_no; }
    int32_t getAbstainCount() const { return m_votes.m_abs; }
    int32_t getNoCount() const { return m_votes.m_no; }
    int32_t getYesCount() const { return m_votes.m_yes; }
    ProposalStatus status(bool is_fundable) const;
    QDateTime collateralDate() const { return m_date_collateral; }
    QDateTime endDate() const { return m_endDate; }
    QDateTime startDate() const { return m_startDate; }
    QString collateralHash() const { return m_hash_collateral; }
    QString hash() const { return m_hash_object; }
    QString parentHash() const { return m_hash_parent; }
    QString paymentAddress() const { return m_address; }
    QString title() const { return m_title; }
    QString toHtml(const BitcoinUnit& unit) const;
    QString toJson() const;
    QString url() const { return m_url; }
    std::optional<int> getFundedHeight() const { return m_funded_height; }
};

using ProposalList = std::vector<std::unique_ptr<Proposal>>;

class ProposalModel : public QAbstractTableModel
{
    Q_OBJECT

private:
    BitcoinUnit m_display_unit{BitcoinUnit::DASH};
    int nAbsVoteReq{0};
    ProposalList m_data;
    QIcon m_icon_failing;
    QIcon m_icon_lapsed;
    QIcon m_icon_passing;
    QIcon m_icon_pending;
    QIcon m_icon_unfunded;
    QIcon m_icon_voting;
    std::array<QIcon, 6> m_icon_confirming;
    Uint256HashSet m_fundable_hashes;

public:
    explicit ProposalModel(QObject* parent = nullptr);

    enum Column : int {
        STATUS = 0,
        TITLE,
        PAYMENT_AMOUNT,
        START_DATE,
        END_DATE,
        VOTING_STATUS,
        HASH,
        _COUNT // for internal use only
    };

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void append(std::unique_ptr<Proposal>&& proposal);
    void remove(int row);
    void reconcile(ProposalList&& proposals, Uint256HashSet&& fundable_hashes);
    void refreshIcons();
    void setDisplayUnit(const BitcoinUnit& display_unit);
    void setVotingParams(int nAbsVoteReq);
    const Proposal* getProposalAt(const QModelIndex& index) const;

private:
    bool isValidRow(int row) const { return !m_data.empty() && row >= 0 && row < rowCount(); }
};

#endif // BITCOIN_QT_PROPOSALMODEL_H
