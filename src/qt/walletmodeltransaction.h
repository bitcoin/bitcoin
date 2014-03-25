// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef WALLETMODELTRANSACTION_H
#define WALLETMODELTRANSACTION_H

#include "walletmodel.h"

#include <QObject>

class Bitcredit_SendCoinsRecipient;

class Bitcredit_CReserveKey;
class Bitcredit_CWallet;
class Bitcredit_CWalletTx;

/** Data model for a walletmodel transaction. */
class Bitcredit_WalletModelTransaction
{
public:
    explicit Bitcredit_WalletModelTransaction(const QList<Bitcredit_SendCoinsRecipient> &recipients);
    ~Bitcredit_WalletModelTransaction();

    QList<Bitcredit_SendCoinsRecipient> getRecipients();

    Bitcredit_CWalletTx *getTransaction();

    void setTransactionFee(qint64 newFee);
    qint64 getTransactionFee();

    qint64 getTotalTransactionAmount();

    void newPossibleKeyChange(Bitcredit_CWallet *wallet);
    Bitcredit_CReserveKey *getPossibleKeyChange();

    //Deposit wallet must be used here. Signing can not occur with locked bitcredit_wallet otherwise.
    void newKeyDepositSignature(Bitcredit_CWallet *deposit_wallet);
    Bitcredit_CReserveKey *getKeyDepositSignature();

    void newKeyRecipient(Bitcredit_CWallet *wallet);
    std::vector<Bitcredit_CReserveKey *> & getKeyRecipients();

private:
    const QList<Bitcredit_SendCoinsRecipient> recipients;
    Bitcredit_CWalletTx *walletTransaction;
    Bitcredit_CReserveKey *keyChange;
    Bitcredit_CReserveKey *keyDepositSignature;
    std::vector<Bitcredit_CReserveKey *> keyRecipients;
    qint64 fee;
};

#endif // WALLETMODELTRANSACTION_H
