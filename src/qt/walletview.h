// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef WALLETVIEW_H
#define WALLETVIEW_H

#include <QStackedWidget>
#include <QPushButton>

class BitcreditGUI;
class ClientModel;
class Credits_OverviewPage;
class Bitcoin_OverviewPage;
class Credits_ReceiveCoinsDialog;
class Bitcoin_ReceiveCoinsDialog;
class Credits_SendCoinsDialog;
class Bitcoin_SendCoinsDialog;
class ClaimCoinsDialog;
class MinerCoinsDialog;
class Bitcredit_SendCoinsRecipient;
class Bitcoin_SendCoinsRecipient;
class Credits_TransactionView;
class Bitcoin_TransactionView;
class MinerDepositsView;
class Bitcredit_WalletModel;
class Bitcoin_WalletModel;

QT_BEGIN_NAMESPACE
class QModelIndex;
class QProgressDialog;
QT_END_NAMESPACE

/*
  WalletView class. This class represents the view to a single wallet.
  It was added to support multiple wallet functionality. Each wallet gets its own WalletView instance.
  It communicates with both the client and the wallet models to give the user an up-to-date view of the
  current core state.
*/
class WalletView : public QStackedWidget
{
    Q_OBJECT

public:
    explicit WalletView(QWidget *parent);
    ~WalletView();

    void setBitcreditGUI(BitcreditGUI *gui);
    /** Set the client model.
        The client model represents the part of the core that communicates with the P2P network, and is wallet-agnostic.
    */
    void setClientModel(ClientModel *clientModel);
    /** Set the wallet model.
        The wallet model represents a bitcoin wallet, and offers access to the list of transactions, address book and sending
        functionality.
    */
    void setWalletModel(Bitcredit_WalletModel *bitcredit_model, Bitcoin_WalletModel *bitcoin_model, Bitcredit_WalletModel *deposit_model);

    bool credits_handlePaymentRequest(const Bitcredit_SendCoinsRecipient& recipient);
    bool bitcoin_handlePaymentRequest(const Bitcoin_SendCoinsRecipient& recipient);

    void credits_showOutOfSyncWarning(bool fShow);
    void bitcoin_showOutOfSyncWarning(bool fShow);

private:
    ClientModel *clientModel;
    Bitcredit_WalletModel *bitcredit_model;
    Bitcoin_WalletModel *bitcoin_model;
    Bitcredit_WalletModel *deposit_model;

    Credits_OverviewPage *credits_overviewPage;
    Bitcoin_OverviewPage *bitcoin_overviewPage;
    QWidget *credits_transactionsPage;
    QWidget *bitcoin_transactionsPage;
    QWidget *minerDepositsPage;
    Credits_ReceiveCoinsDialog *credits_receiveCoinsPage;
    Bitcoin_ReceiveCoinsDialog *bitcoin_receiveCoinsPage;
    Credits_SendCoinsDialog *credits_sendCoinsPage;
    Bitcoin_SendCoinsDialog *bitcoin_sendCoinsPage;
    ClaimCoinsDialog *claimCoinsPage;
    MinerCoinsDialog *minerCoinsPage;

    Credits_TransactionView *credits_transactionView;
    Bitcoin_TransactionView *bitcoin_transactionView;
    MinerDepositsView *minerDepositsView;

    QProgressDialog *progressDialog;

    QPushButton * CreateButton(QWidget * view, QWidget * page);
    void CreateLowerLayout(QWidget * view, QWidget * page);

public slots:
    /** Switch to overview (home) page */
    void credits_gotoOverviewPage();
    void bitcoin_gotoOverviewPage();
    /** Switch to new claim  coins page */
    void gotoClaimCoinsPage();
    /** Switch to history (transactions) page */
    void credits_gotoHistoryPage();
    void bitcoin_gotoHistoryPage();
    /** Switch to miner deposits page */
    void gotoMinerDepositsPage();
    /** Switch to receive coins page */
    void credits_gotoReceiveCoinsPage();
    void bitcoin_gotoReceiveCoinsPage();
    /** Switch to send coins page */
    void credits_gotoSendCoinsPage(QString addr = "");
    void bitcoin_gotoSendCoinsPage(QString addr = "");
    /** Switch to miner coins page */
    void gotoMinerCoinsPage(QString addr = "");

    /** Show Sign/Verify Message dialog and switch to sign message tab */
    void gotoSignMessageTab(QString addr = "");
    /** Show Sign/Verify Message dialog and switch to verify message tab */
    void gotoVerifyMessageTab(QString addr = "");

    /** Show incoming transaction notification for new transactions.

        The new items are those between start and end inclusive, under the given parent item.
    */
    void credits_processNewTransaction(const QModelIndex& parent, int start, int /*end*/);
    void bitcoin_processNewTransaction(const QModelIndex& parent, int start, int /*end*/);
    /** Encrypt the wallet */
    void bitcredit_encryptWallet(bool status);
    void bitcoin_encryptWallet(bool status);
    void deposit_encryptWallet(bool status);
    /** Backup the wallet */
    void bitcredit_backupWallet();
    void bitcoin_backupWallet();
    void deposit_backupWallet();
    /** Change encrypted wallet passphrase */
    void bitcredit_changePassphrase();
    void bitcoin_changePassphrase();
    void deposit_changePassphrase();
    /** Ask for passphrase to unlock wallet temporarily */
    void bitcredit_unlockWallet();
    void bitcoin_unlockWallet();
    void deposit_unlockWallet();

    /** Show used sending addresses */
    void credits_usedSendingAddresses();
    void bitcoin_usedSendingAddresses();
    /** Show used receiving addresses */
    void credits_usedReceivingAddresses();
    void bitcoin_usedReceivingAddresses();

    void bitcredit_setNumBlocks(int count);

    /** Re-emit encryption status signal */
    void updateEncryptionStatus();

    /** Show progress dialog e.g. for rescan */
    void showProgress(const QString &title, int nProgress);

signals:
    /** Signal that we want to show the main window */
    void showNormalIfMinimized();
    /**  Fired when a message should be reported to the user */
    void message(const QString &title, const QString &message, unsigned int style);
    /** Encryption status of wallet changed */
    void bitcredit_encryptionStatusChanged(int status);
    void bitcoin_encryptionStatusChanged(int status);
    void deposit_encryptionStatusChanged(int status);
    /** Notify that a new transaction appeared */
    void incomingTransaction(const QString& date, int unit, qint64 amount, const QString& type, const QString& address);
};

#endif // WALLETVIEW_H
