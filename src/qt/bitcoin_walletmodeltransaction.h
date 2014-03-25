// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLETMODELTRANSACTION_H
#define BITCOIN_WALLETMODELTRANSACTION_H

#include "bitcoin_walletmodel.h"

#include <QObject>

class Bitcoin_SendCoinsRecipient;

class Bitcoin_CReserveKey;
class Bitcoin_CWallet;
class Bitcoin_CWalletTx;

/** Data model for a walletmodel transaction. */
class Bitcoin_WalletModelTransaction
{
public:
    explicit Bitcoin_WalletModelTransaction(const QList<Bitcoin_SendCoinsRecipient> &recipients);
    ~Bitcoin_WalletModelTransaction();

    QList<Bitcoin_SendCoinsRecipient> getRecipients();

    Bitcoin_CWalletTx *getTransaction();

    void setTransactionFee(qint64 newFee);
    qint64 getTransactionFee();

    qint64 getTotalTransactionAmount();

    void newPossibleKeyChange(Bitcoin_CWallet *wallet);
    Bitcoin_CReserveKey *getPossibleKeyChange();

private:
    const QList<Bitcoin_SendCoinsRecipient> recipients;
    Bitcoin_CWalletTx *walletTransaction;
    Bitcoin_CReserveKey *keyChange;
    qint64 fee;
};

#endif // WALLETMODELTRANSACTION_H
