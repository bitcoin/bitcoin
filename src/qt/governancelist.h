// Copyright (c) 2021-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_GOVERNANCELIST_H
#define BITCOIN_QT_GOVERNANCELIST_H

#include <primitives/transaction.h>
#include <pubkey.h>

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

class ClientModel;
class ProposalModel;
class WalletModel;
class ProposalWizard;

class CDeterministicMNList;
enum vote_outcome_enum_t : int;

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

#endif // BITCOIN_QT_GOVERNANCELIST_H
