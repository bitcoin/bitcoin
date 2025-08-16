// Copyright (c) 2021-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_GOVERNANCELIST_H
#define BITCOIN_QT_GOVERNANCELIST_H

#include <governance/object.h>
#include <governance/vote.h>
#include <primitives/transaction.h>
#include <qt/bitcoinunits.h>
#include <sync.h>
#include <util/system.h>

#include <QAbstractTableModel>
#include <QDateTime>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QWidget>

#include <map>
#include <memory>

inline constexpr int GOVERNANCELIST_UPDATE_SECONDS = 10;

namespace Ui {
class GovernanceList;
}

class CDeterministicMNList;
class ClientModel;
class ProposalModel;
class WalletModel;
class ProposalWizard;

/** Governance Manager page widget */
class GovernanceList : public QWidget
{
    Q_OBJECT

public:
    explicit GovernanceList(QWidget* parent = nullptr);
    ~GovernanceList() override;
    void setClientModel(ClientModel* clientModel);
    void setWalletModel(WalletModel* walletModel);

private:
    ClientModel* clientModel{nullptr};
    WalletModel* walletModel{nullptr};

    std::unique_ptr<Ui::GovernanceList> ui;
    ProposalModel* proposalModel;
    QSortFilterProxyModel* proposalModelProxy;

    QMenu* proposalContextMenu;
    QTimer* timer;
    std::unique_ptr<ProposalWizard> proposalWizard;

    // Voting-related members
    std::map<uint256, CKeyID> votableMasternodes; // proTxHash -> voting keyID

    void updateVotingCapability();
    bool canVote() const { return !votableMasternodes.empty(); }
    void voteForProposal(vote_outcome_enum_t outcome);

private Q_SLOTS:
    void updateDisplayUnit();
    void updateProposalList();
    void updateProposalCount() const;
    void updateMasternodeCount() const;
    void showProposalContextMenu(const QPoint& pos);
    void showAdditionalInfo(const QModelIndex& index);
    void showCreateProposalDialog();

    // Voting slots
    void voteYes();
    void voteNo();
    void voteAbstain();
};

class Proposal : public QObject
{
private:
    Q_OBJECT

    ClientModel* clientModel;
    const CGovernanceObject govObj;

    QString m_title;
    QDateTime m_startDate;
    QDateTime m_endDate;
    double m_paymentAmount;
    QString m_url;

public:
    explicit Proposal(ClientModel* _clientModel, const CGovernanceObject& _govObj, QObject* parent = nullptr);
    QString title() const;
    QString hash() const;
    QDateTime startDate() const;
    QDateTime endDate() const;
    double paymentAmount() const;
    QString url() const;
    bool isActive() const;
    QString votingStatus(int nAbsVoteReq) const;
    int GetAbsoluteYesCount() const;

    void openUrl() const;

    QString toJson() const;
};

class ProposalModel : public QAbstractTableModel
{
    Q_OBJECT

private:
    QList<const Proposal*> m_data;
    int nAbsVoteReq = 0;
    BitcoinUnit m_display_unit{BitcoinUnit::DASH};

public:
    explicit ProposalModel(QObject* parent = nullptr) :
        QAbstractTableModel(parent){};

    enum Column : int {
        HASH = 0,
        TITLE,
        START_DATE,
        END_DATE,
        PAYMENT_AMOUNT,
        IS_ACTIVE,
        VOTING_STATUS,
        _COUNT // for internal use only
    };

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    static int columnWidth(int section);
    void append(const Proposal* proposal);
    void remove(int row);
    void reconcile(Span<const Proposal*> proposals);
    void setVotingParams(int nAbsVoteReq);

    const Proposal* getProposalAt(const QModelIndex& index) const;

    void setDisplayUnit(BitcoinUnit display_unit);
};

#endif // BITCOIN_QT_GOVERNANCELIST_H
