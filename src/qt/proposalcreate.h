// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PROPOSALCREATE_H
#define BITCOIN_QT_PROPOSALCREATE_H

#include <QByteArray>
#include <QDialog>
#include <QString>

class WalletModel;

namespace Ui {
class ProposalCreate;
} // namespace Ui

class ProposalCreate : public QDialog
{
    Q_OBJECT
public:
    explicit ProposalCreate(WalletModel* walletModel, QWidget* parent = nullptr);
    ~ProposalCreate();

private Q_SLOTS:
    void onCreate();
    void onViewJson();
    void onViewPayload();

    void updateLabels();
    void updateDisplayUnit();
    void validateFields();

private:
    WalletModel* m_walletModel;
    Ui::ProposalCreate* m_ui;

    // State
    int m_relay_confs{0};
    int64_t m_superblock_cycle{0};
    int64_t m_target_spacing{0};
    QString m_fee_formatted;
    QString m_hex;
    QString m_json;

    void buildJsonAndHex();
};

#endif // BITCOIN_QT_PROPOSALCREATE_H
