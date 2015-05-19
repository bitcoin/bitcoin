// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef WALLETMODELTRANSACTION_H
#define WALLETMODELTRANSACTION_H

#include "walletmodel.h"

#include <QObject>

class Bitcredit_SendCoinsRecipient;

class Credits_CReserveKey;
class Credits_CWallet;
class Credits_CWalletTx;

/** Data model for a walletmodel transaction. */
class Bitcredit_WalletModelTransaction
{
public:
    explicit Bitcredit_WalletModelTransaction(const QList<Bitcredit_SendCoinsRecipient> &recipients);
    ~Bitcredit_WalletModelTransaction();

    QList<Bitcredit_SendCoinsRecipient> getRecipients();

    Credits_CWalletTx *getTransaction();

    void setTransactionFee(qint64 newFee);
    qint64 getTransactionFee();

    qint64 getTotalTransactionAmount();

    void newPossibleKeyChange(Credits_CWallet *wallet);
    Credits_CReserveKey *getPossibleKeyChange();

    //Deposit wallet must be used here. Signing can not occur with locked credits_wallet otherwise.
    void newKeyDepositSignature(Credits_CWallet *deposit_wallet);
    Credits_CReserveKey *getKeyDepositSignature();

    void newKeyRecipient(Credits_CWallet *wallet);
    std::vector<Credits_CReserveKey *> & getKeyRecipients();

private:
    const QList<Bitcredit_SendCoinsRecipient> recipients;
    Credits_CWalletTx *walletTransaction;
    Credits_CReserveKey *keyChange;
    Credits_CReserveKey *keyDepositSignature;
    std::vector<Credits_CReserveKey *> keyRecipients;
    qint64 fee;
};

#endif // WALLETMODELTRANSACTION_H
