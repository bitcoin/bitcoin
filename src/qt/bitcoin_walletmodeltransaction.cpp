// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoin_walletmodeltransaction.h"

#include "bitcoin_wallet.h"

Bitcoin_WalletModelTransaction::Bitcoin_WalletModelTransaction(const QList<Bitcoin_SendCoinsRecipient> &recipients) :
    recipients(recipients),
    walletTransaction(0),
    keyChange(0),
    fee(0)
{
    walletTransaction = new Bitcoin_CWalletTx();
}

Bitcoin_WalletModelTransaction::~Bitcoin_WalletModelTransaction()
{
    delete keyChange;
    delete walletTransaction;
}

QList<Bitcoin_SendCoinsRecipient> Bitcoin_WalletModelTransaction::getRecipients()
{
    return recipients;
}

Bitcoin_CWalletTx *Bitcoin_WalletModelTransaction::getTransaction()
{
    return walletTransaction;
}

qint64 Bitcoin_WalletModelTransaction::getTransactionFee()
{
    return fee;
}

void Bitcoin_WalletModelTransaction::setTransactionFee(qint64 newFee)
{
    fee = newFee;
}

qint64 Bitcoin_WalletModelTransaction::getTotalTransactionAmount()
{
    qint64 totalTransactionAmount = 0;
    foreach(const Bitcoin_SendCoinsRecipient &rcp, recipients)
    {
        totalTransactionAmount += rcp.amount;
    }
    return totalTransactionAmount;
}

void Bitcoin_WalletModelTransaction::newPossibleKeyChange(Bitcoin_CWallet *wallet)
{
    keyChange = new Bitcoin_CReserveKey(wallet);
}

Bitcoin_CReserveKey *Bitcoin_WalletModelTransaction::getPossibleKeyChange()
{
    return keyChange;
}
