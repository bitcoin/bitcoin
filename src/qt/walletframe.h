// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef WALLETFRAME_H
#define WALLETFRAME_H

#include <QFrame>
#include <QMap>

class BitcreditGUI;
class ClientModel;
class Bitcredit_SendCoinsRecipient;
class Bitcoin_SendCoinsRecipient;
class Bitcredit_WalletModel;
class Bitcoin_WalletModel;
class WalletView;

QT_BEGIN_NAMESPACE
class QStackedWidget;
QT_END_NAMESPACE

class WalletFrame : public QFrame
{
    Q_OBJECT

public:
    explicit WalletFrame(BitcreditGUI *_gui = 0);
    ~WalletFrame();

    void setClientModel(ClientModel *clientModel);

    bool addWallet(const QString& name, Bitcredit_WalletModel *bitcredit_model, Bitcoin_WalletModel *bitcoin_model, Bitcredit_WalletModel *deposit_model);
    bool setCurrentWallet(const QString& name);
    bool removeWallet(const QString &name);
    void removeAllWallets();

    bool credits_handlePaymentRequest(const Bitcredit_SendCoinsRecipient& recipient);
    bool bitcoin_handlePaymentRequest(const Bitcoin_SendCoinsRecipient& recipient);

    void credits_showOutOfSyncWarning(bool fShow);
    void bitcoin_showOutOfSyncWarning(bool fShow);

private:
    QStackedWidget *walletStack;
    BitcreditGUI *gui;
    ClientModel *clientModel;
    QMap<QString, WalletView*> mapWalletViews;

    bool credits_bOutOfSync;
    bool bitcoin_bOutOfSync;

    WalletView *currentWalletView();

public slots:
    /** Switch to overview (home) page */
    void credits_gotoOverviewPage();
    void bitcoin_gotoOverviewPage();
    /** Switch to new claim coins page */
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
    /** Switch to send coins page */
    void gotoMinerCoinsPage(QString addr = "");

    /** Show Sign/Verify Message dialog and switch to sign message tab */
    void gotoSignMessageTab(QString addr = "");
    /** Show Sign/Verify Message dialog and switch to verify message tab */
    void gotoVerifyMessageTab(QString addr = "");

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
};

#endif // WALLETFRAME_H
