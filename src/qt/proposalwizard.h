// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PROPOSALWIZARD_H
#define BITCOIN_QT_PROPOSALWIZARD_H

#include <qt/forms/ui_proposalwizard.h>

#include <QCloseEvent>
#include <QDialog>
#include <QObject>
#include <QString>

class QTimer;

namespace interfaces {
class Node;
}
namespace Ui {
class SendCoinsEntry;
}

class WalletModel;
// The UI header is included above for complete type to satisfy unique_ptr deleter

class ProposalWizard : public QDialog
{
    Q_OBJECT
public:
    explicit ProposalWizard(interfaces::Node& node, WalletModel* walletModel, QWidget* parent = nullptr);
    ~ProposalWizard();

private Q_SLOTS:
    void onNextFromDetails();
    void onBackToDetails();
    void onValidateJson();
    void onNextFromReview();
    void onBackToReview();
    void onPrepare();
    void onMaybeAdvanceAfterConfirmations();
    void onSubmit();
    void onGoToSubmit();

    void updateLabels();
    void updateDisplayUnit();

private:
    interfaces::Node& m_node;
    WalletModel* m_walletModel;
    Ui::ProposalWizard* m_ui;

    // State
    QString m_hex;
    QString m_txid;
    QString m_fee_formatted;
    qint64 m_prepareTime{0};
    int m_relayRequiredConfs{1};
    int m_requiredConfs{6};
    int m_lastConfs{-1};
    bool m_submitted{false};
    QTimer* m_confirmTimer{nullptr};

    void buildJsonAndHex();
    int queryConfirmations(const QString& txid);
    void closeEvent(QCloseEvent* event) override;
};

#endif // BITCOIN_QT_PROPOSALWIZARD_H
