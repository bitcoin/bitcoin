// Copyright (c) 2021-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PROPOSALLIST_H
#define BITCOIN_QT_PROPOSALLIST_H

#include <interfaces/node.h>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <saltedhasher.h>

#include <qt/proposalmodel.h>

#include <QMenu>
#include <QSortFilterProxyModel>
#include <QWidget>

#include <map>
#include <optional>
#include <vector>

class ClientModel;
class MasternodeFeed;
class ProposalCreate;
class ProposalFeed;
class ProposalModel;
class WalletModel;
enum vote_outcome_enum_t : int;
struct ProposalData;
namespace Governance {
class Object;
} // namespace Governance
namespace Ui {
class ProposalList;
} // namespace Ui

enum class ProposalSource : uint8_t {
    Active,
    Local
};

/** Proposal list widget */
class ProposalList : public QWidget
{
    Q_OBJECT

    Ui::ProposalList* ui;

public:
    explicit ProposalList(QWidget* parent = nullptr);
    ~ProposalList() override;
    void setClientModel(ClientModel* clientModel);
    void setWalletModel(WalletModel* walletModel);

private:
    ClientModel* clientModel{nullptr};
    interfaces::GOV::GovernanceInfo m_gov_info;
    MasternodeFeed* m_feed_masternode{nullptr};
    ProposalFeed* m_feed_proposal{nullptr};
    ProposalModel* proposalModel{nullptr};
    ProposalSource m_proposal_source{ProposalSource::Active};
    QMenu* proposalContextMenu{nullptr};
    QSortFilterProxyModel* proposalModelProxy{nullptr};
    std::atomic<bool> m_col_refresh{false};
    Uint256HashMap<CKeyID> votableMasternodes;
    WalletModel* walletModel{nullptr};

    bool canVote() const { return !votableMasternodes.empty(); }
    int queryCollateralDepth(const uint256& collateralHash) const;
    std::vector<Governance::Object> getWalletProposals(std::optional<bool> pending) const;
    void refreshColumnWidths();
    void requestForceRefresh();
    void setProposalList(ProposalData&& data);
    void updateEmptyPagePalette();
    void updateEmptyState();
    void updateInfoTooltip();
    void updateProposalButtons();
    void voteForProposal(vote_outcome_enum_t outcome);

protected:
    void changeEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private Q_SLOTS:
    void copyProposalJson();
    void openProposalUrl();
    void setProposalSource(int index);
    void showAdditionalInfo(const QModelIndex& index);
    void showCreateProposalDialog();
    void showProposalContextMenu(const QPoint& pos);
    void showResumeProposalDialog();
    void updateDisplayUnit();
    void updateProposalCount();
    void updateProposalList();

    // Voting slots
    void voteYes();
    void voteNo();
    void voteAbstain();

Q_SIGNALS:
    void showProposalInfo();
};

#endif // BITCOIN_QT_PROPOSALLIST_H
