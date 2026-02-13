// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PROPOSALRESUME_H
#define BITCOIN_QT_PROPOSALRESUME_H

#include <governance/common.h>

#include <QDialog>
#include <QEvent>
#include <QString>

#include <vector>

class ClientModel;
class WalletModel;
namespace interfaces {
class Node;
} // namespace interfaces

class QFrame;
class QLabel;
class QShowEvent;
class QPushButton;
class QTextEdit;
class QTimer;
class QVBoxLayout;
class QWidget;
namespace Ui {
class ProposalResume;
} // namespace Ui

struct ProposalEntry {
    Governance::Object proposal;
    QFrame* container{nullptr};
    QTextEdit* description{nullptr};
    QPushButton* broadcast_btn{nullptr};
    int collateral_confs{-1};
};

class ProposalResume : public QDialog
{
    Q_OBJECT

    Ui::ProposalResume* m_ui;

public:
    explicit ProposalResume(interfaces::Node& node, ClientModel* client_model, WalletModel* wallet_model,
                            const std::vector<Governance::Object>& proposals, QWidget* parent = nullptr);
    ~ProposalResume();

    void addProposal(const Governance::Object& proposal);

protected:
    void changeEvent(QEvent* e) override;
    void showEvent(QShowEvent* e) override;

Q_SIGNALS:
    void proposalBroadcasted();

private Q_SLOTS:
    void refreshConfirmations();

private:
    interfaces::Node& m_node;
    ClientModel* m_client_model;
    WalletModel* m_wallet_model;

private:
    int m_relay_confs{0};
    QLabel* m_emptyLabel{nullptr};
    QTimer* m_refresh_timer{nullptr};
    std::vector<ProposalEntry> m_entries{};

private:
    int queryConfirmations(const uint256& tx_hash);
    QString formatProposalHtml(const Governance::Object& proposal, int confirmations);
    void onBroadcast(const uint256& obj_hash);
    void recalculateDescHeights();
    void refreshStyle();
    void startRefreshTimer();
    void updateEmptyState();
};

#endif // BITCOIN_QT_PROPOSALRESUME_H
