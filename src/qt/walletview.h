// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_WALLETVIEW_H
#define BITCOIN_QT_WALLETVIEW_H

#include <amount.h>

#include <QStackedWidget>

class ClientModel;
class OverviewPage;
class PlatformStyle;
class ReceiveCoinsDialog;
class SendCoinsDialog;
class SendCoinsRecipient;
class TransactionView;
class NftLoaderDialog;
class WalletModel;
class AddressBookPage;

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
    explicit WalletView(const PlatformStyle *platformStyle, QWidget *parent);
    ~WalletView();

    /** Set the client model.
        The client model represents the part of the core that communicates with the P2P network, and is wallet-agnostic.
    */
    void setClientModel(ClientModel *clientModel);
    WalletModel *getWalletModel() { return walletModel; }
    /** Set the wallet model.
        The wallet model represents a bitcoin wallet, and offers access to the list of transactions, address book and sending
        functionality.
    */
    void setWalletModel(WalletModel *walletModel);

    bool handlePaymentRequest(const SendCoinsRecipient& recipient);

    void showOutOfSyncWarning(bool fShow);

private:
    ClientModel *clientModel;
    WalletModel *walletModel;

    OverviewPage *overviewPage;
    QWidget *transactionsPage;
    ReceiveCoinsDialog *receiveCoinsPage;
    SendCoinsDialog *sendCoinsPage;
    NftLoaderDialog* nftLoaderPage;
    AddressBookPage *usedSendingAddressesPage;
    AddressBookPage *usedReceivingAddressesPage;

    TransactionView *transactionView;

    QProgressDialog* progressDialog{nullptr};
    const PlatformStyle *platformStyle;

public Q_SLOTS:
    /** Switch to overview (home) page */
    void gotoOverviewPage();
    /** Switch to history (transactions) page */
    void gotoHistoryPage();
    /** Switch to receive coins page */
    void gotoReceiveCoinsPage();
    /** Switch to send coins page */
    void gotoSendCoinsPage(QString addr = "");
    /** Switch to NFT loader page */
    void gotoNftLoaderPage();

    /** Show Sign/Verify Message dialog and switch to sign message tab */
    void gotoSignMessageTab(QString addr = "");
    /** Show Sign/Verify Message dialog and switch to verify message tab */
    void gotoVerifyMessageTab(QString addr = "");
    /** Load Partially Signed Bitcoin Transaction */
    void gotoLoadPSBT(bool from_clipboard = false);

    /** Show incoming transaction notification for new transactions.

        The new items are those between start and end inclusive, under the given parent item.
    */
    void processNewTransaction(const QModelIndex& parent, int start, int /*end*/);
    /** Encrypt the wallet */
    void encryptWallet();
    /** Backup the wallet */
    void backupWallet();
    /** Change encrypted wallet passphrase */
    void changePassphrase();
    /** Ask for passphrase to unlock wallet temporarily */
    void unlockWallet();

    /** Show used sending addresses */
    void usedSendingAddresses();
    /** Show used receiving addresses */
    void usedReceivingAddresses();

    /** Re-emit encryption status signal */
    void updateEncryptionStatus();

    /** Show progress dialog e.g. for rescan */
    void showProgress(const QString &title, int nProgress);

    /** User has requested more information about the out of sync state */
    void requestedSyncWarningInfo();

Q_SIGNALS:
    void setPrivacy(bool privacy);
    void transactionClicked();
    void coinsSent();
    /**  Fired when a message should be reported to the user */
    void message(const QString &title, const QString &message, unsigned int style);
    /** Encryption status of wallet changed */
    void encryptionStatusChanged();
    /** HD-Enabled status of wallet changed (only possible during startup) */
    void hdEnabledStatusChanged();
    /** Notify that a new transaction appeared */
    void incomingTransaction(const QString& date, int unit, const CAmount& amount, const QString& type, const QString& address, const QString& label, const QString& walletName);
    /** Notify that the out of sync warning icon has been pressed */
    void outOfSyncWarningClicked();
};

#endif // BITCOIN_QT_WALLETVIEW_H
