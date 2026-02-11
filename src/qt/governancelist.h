// Copyright (c) 2021-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_GOVERNANCELIST_H
#define BITCOIN_QT_GOVERNANCELIST_H

#include <primitives/transaction.h>
#include <pubkey.h>
#include <saltedhasher.h>

#include <qt/proposalmodel.h>

#include <QMenu>
#include <QSortFilterProxyModel>
#include <QTimer>
#include <QThread>
#include <QWidget>

#include <atomic>
#include <map>

inline constexpr int GOVERNANCELIST_UPDATE_SECONDS = 10;

class ClientModel;
class ProposalModel;
class WalletModel;
class ProposalWizard;
class CDeterministicMNList;
enum vote_outcome_enum_t : int;
namespace Ui {
class GovernanceList;
} // namespace Ui

/** Governance Manager page widget */
class GovernanceList : public QWidget
{
    Q_OBJECT

    Ui::GovernanceList* ui;

public:
    explicit GovernanceList(QWidget* parent = nullptr);
    ~GovernanceList() override;
    void setClientModel(ClientModel* clientModel);
    void setWalletModel(WalletModel* walletModel);

private:
    struct CalcProposalList {
        int m_abs_vote_req{0};
        ProposalList m_proposals;
        Uint256HashMap<CKeyID> m_votable_masternodes;
    };

    ClientModel* clientModel{nullptr};
    ProposalModel* proposalModel{nullptr};
    QMenu* proposalContextMenu{nullptr};
    QObject* m_worker{nullptr};
    QSortFilterProxyModel* proposalModelProxy{nullptr};
    QThread* m_thread{nullptr};
    QTimer* m_timer{nullptr};
    std::atomic<bool> m_col_refresh{false};
    std::atomic<bool> m_in_progress{false};
    Uint256HashMap<CKeyID> votableMasternodes;
    WalletModel* walletModel{nullptr};

    bool canVote() const { return !votableMasternodes.empty(); }
    CalcProposalList calcProposalList() const;
    void handleProposalListChanged();
    void refreshColumnWidths();
    void setProposalList(CalcProposalList&& data);
    void voteForProposal(vote_outcome_enum_t outcome);

protected:
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private Q_SLOTS:
    void updateDisplayUnit();
    void updateProposalList();
    void updateProposalCount();
    void updateMasternodeCount() const;
    void showProposalContextMenu(const QPoint& pos);
    void showAdditionalInfo(const QModelIndex& index);
    void showCreateProposalDialog();
    void openProposalUrl();
    void copyProposalJson();

    // Voting slots
    void voteYes();
    void voteNo();
    void voteAbstain();
};

#endif // BITCOIN_QT_GOVERNANCELIST_H
