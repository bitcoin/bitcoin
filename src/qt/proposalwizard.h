// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_PROPOSALWIZARD_H
#define BITCOIN_QT_PROPOSALWIZARD_H

#include <QDialog>
#include <QObject>
#include <QString>
#include <qt/forms/ui_proposalwizard.h>
#include <memory>

class QDateTimeEdit;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QComboBox;
class QPlainTextEdit;
class QPushButton;
class QStackedWidget;
class QProgressBar;
class QTimer;

namespace interfaces { class Node; }
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

private:
    interfaces::Node& m_node;
    WalletModel* m_walletModel;
    std::unique_ptr<Ui::ProposalWizard> m_ui;

    // UI pointers
    QStackedWidget* stacked{nullptr};
    QLineEdit* editName{nullptr};
    QLineEdit* editUrl{nullptr};
    QLineEdit* editPayAddr{nullptr};
    QLineEdit* editTxid{nullptr};
    QComboBox* comboFirstPayment{nullptr};
    QComboBox* comboPayments{nullptr};
    QDoubleSpinBox* spinAmount{nullptr};
    QLabel* labelFeeValue{nullptr};
    QLabel* labelTotalValue{nullptr};
    QLabel* labelSubheader{nullptr};
    QLabel* labelPrepare{nullptr};
    QPushButton* btnNext1{nullptr};
    QPushButton* btnPrepare{nullptr};

    QPlainTextEdit* plainJson{nullptr};
    QLineEdit* editHex{nullptr};
    QLabel* labelValidateStatus{nullptr};
    QPushButton* btnNext2{nullptr};

    QLabel* labelTxid{nullptr};
    QLabel* labelConf{nullptr};
    QLabel* labelConfStatus{nullptr};
    QLabel* labelEta{nullptr};
    QProgressBar* progressConfirmations{nullptr};
    QLabel* labelConfStatus2{nullptr};
    QLabel* labelEta2{nullptr};
    QProgressBar* progressConfirmations2{nullptr};
    QPushButton* btnNext3{nullptr};

    QLabel* labelGovObjId{nullptr};
    QLineEdit* editGovObjId{nullptr};
    QPushButton* btnCopyGovId{nullptr};
    QPushButton* btnSubmit{nullptr};

    // State
    QString m_hex;
    QString m_txid;
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


