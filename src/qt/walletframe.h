// Copyright (c) 2011-2019 The Bitcointalkcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOINTALKCOIN_QT_WALLETFRAME_H
#define BITCOINTALKCOIN_QT_WALLETFRAME_H

#include <QFrame>
#include <QMap>
#include <qt/messagemodel.h>

class BitcointalkcoinGUI;
class ClientModel;
class PlatformStyle;
class SendCoinsRecipient;
class WalletModel;
class WalletView;
class MessageModel;

QT_BEGIN_NAMESPACE
class QStackedWidget;
QT_END_NAMESPACE

/**
 * A container for embedding all wallet-related
 * controls into BitcointalkcoinGUI. The purpose of this class is to allow future
 * refinements of the wallet controls with minimal need for further
 * modifications to BitcointalkcoinGUI, thus greatly simplifying merges while
 * reducing the risk of breaking top-level stuff.
 */
class WalletFrame : public QFrame
{
    Q_OBJECT

public:
    explicit WalletFrame(const PlatformStyle *platformStyle, BitcointalkcoinGUI *_gui = nullptr);
    ~WalletFrame();

    void setClientModel(ClientModel *clientModel);

    void addWallet(WalletModel *walletModel);
    void setCurrentWallet(WalletModel* wallet_model);
    void removeWallet(WalletModel* wallet_model);
    void removeAllWallets();

    bool handlePaymentRequest(const SendCoinsRecipient& recipient);

    void showOutOfSyncWarning(bool fShow);

Q_SIGNALS:
    /** Notify that the user has requested more information about the out-of-sync warning */
    void requestedSyncWarningInfo();

private:
    QStackedWidget *walletStack;
    BitcointalkcoinGUI *gui;
    ClientModel *clientModel;
#ifdef ENABLE_SECURE_MESSAGING
    MessageModel *messageModel;
#endif
    QMap<WalletModel*, WalletView*> mapWalletViews;

    bool bOutOfSync;

    const PlatformStyle *platformStyle;

public:
    WalletView* currentWalletView() const;
    WalletModel* currentWalletModel() const;

public Q_SLOTS:
    /** Switch to overview (home) page */
    void gotoOverviewPage();
    /** Switch to history (transactions) page */
    void gotoHistoryPage();
    /** Switch to receive coins page */
    void gotoReceiveCoinsPage();
    /** Switch to send coins page */
    void gotoSendCoinsPage(QString addr = "");

    /** Show Sign/Verify Message dialog and switch to sign message tab */
    void gotoSignMessageTab(QString addr = "");
    /** Show Sign/Verify Message dialog and switch to verify message tab */
    void gotoVerifyMessageTab(QString addr = "");
#ifdef ENABLE_SECURE_MESSAGING
	void gotoSendMessagesPage();

    void gotoMessagesPage();
#endif
    /** Encrypt the wallet */
    void encryptWallet(bool status);
    /** Backup the wallet */
    void backupWallet();
    /** Change encrypted wallet passphrase */
    void changePassphrase();
    /** Ask for passphrase to unlock wallet temporarily */
    void unlockWallet();

    /** Lock the wallet */
    void lockWallet();

    /** Show used sending addresses */
    void usedSendingAddresses();
    /** Show used receiving addresses */
    void usedReceivingAddresses();
    /** Pass on signal over requested out-of-sync-warning information */
    void outOfSyncWarningClicked();
};

#endif // BITCOINTALKCOIN_QT_WALLETFRAME_H
