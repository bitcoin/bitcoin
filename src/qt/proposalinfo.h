// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PROPOSALINFO_H
#define BITCOIN_QT_PROPOSALINFO_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <QWidget>

class ClientModel;
class MasternodeFeed;
class ProposalFeed;
class WalletModel;
namespace Ui {
class ProposalInfo;
} // namespace Ui

QT_BEGIN_NAMESPACE
class QEvent;
QT_END_NAMESPACE

class ProposalInfo : public QWidget
{
    Q_OBJECT

    Ui::ProposalInfo* ui;

public:
    explicit ProposalInfo(QWidget* parent = nullptr);
    ~ProposalInfo() override;

    void setClientModel(ClientModel* model);
#ifdef ENABLE_WALLET
    void setWalletModel(WalletModel* model);
#endif // ENABLE_WALLET

public Q_SLOTS:
    void updateProposalInfo();

protected:
    void changeEvent(QEvent* e) override;

private:
    ClientModel* m_client_model{nullptr};
    MasternodeFeed* m_feed_masternode{nullptr};
    ProposalFeed* m_feed_proposal{nullptr};
#ifdef ENABLE_WALLET
    WalletModel* m_wallet_model{nullptr};
#endif // ENABLE_WALLET
};

#endif // BITCOIN_QT_PROPOSALINFO_H
