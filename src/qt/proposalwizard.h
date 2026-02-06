// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PROPOSALWIZARD_H
#define BITCOIN_QT_PROPOSALWIZARD_H

#include <qt/forms/ui_proposalwizard.h>

#include <QByteArray>
#include <QDialog>
#include <QString>

class WalletModel;

class ProposalWizard : public QDialog
{
    Q_OBJECT
public:
    explicit ProposalWizard(WalletModel* walletModel, QWidget* parent = nullptr);
    ~ProposalWizard();

private Q_SLOTS:
    void onCreate();
    void onViewJson();
    void onViewPayload();

    void updateLabels();
    void updateDisplayUnit();
    void validateFields();

private:
    WalletModel* m_walletModel;
    Ui::ProposalWizard* m_ui;

    // State
    int m_relay_confs{0};
    int64_t m_superblock_cycle{0};
    int64_t m_target_spacing{0};
    QString m_fee_formatted;
    QString m_hex;
    QString m_json;

    void buildJsonAndHex();
};

#endif // BITCOIN_QT_PROPOSALWIZARD_H
