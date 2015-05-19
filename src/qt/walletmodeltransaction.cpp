// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "walletmodeltransaction.h"

#include "wallet.h"

Bitcredit_WalletModelTransaction::Bitcredit_WalletModelTransaction(const QList<Bitcredit_SendCoinsRecipient> &recipients) :
    recipients(recipients),
    walletTransaction(0),
    keyChange(0),
    keyDepositSignature(0),
    keyRecipients(),
    fee(0)
{
    walletTransaction = new Credits_CWalletTx();
}

Bitcredit_WalletModelTransaction::~Bitcredit_WalletModelTransaction()
{
    while (!keyRecipients.empty()){
    	Credits_CReserveKey* key = keyRecipients.back();
    	keyRecipients.pop_back();
        delete key;
    }
    delete keyDepositSignature;
    delete keyChange;
    delete walletTransaction;
}

QList<Bitcredit_SendCoinsRecipient> Bitcredit_WalletModelTransaction::getRecipients()
{
    return recipients;
}

Credits_CWalletTx *Bitcredit_WalletModelTransaction::getTransaction()
{
    return walletTransaction;
}

qint64 Bitcredit_WalletModelTransaction::getTransactionFee()
{
    return fee;
}

void Bitcredit_WalletModelTransaction::setTransactionFee(qint64 newFee)
{
    fee = newFee;
}

qint64 Bitcredit_WalletModelTransaction::getTotalTransactionAmount()
{
    qint64 totalTransactionAmount = 0;
    foreach(const Bitcredit_SendCoinsRecipient &rcp, recipients)
    {
        totalTransactionAmount += rcp.amount;
    }
    return totalTransactionAmount;
}

void Bitcredit_WalletModelTransaction::newPossibleKeyChange(Credits_CWallet *wallet)
{
    keyChange = new Credits_CReserveKey(wallet);
}

Credits_CReserveKey *Bitcredit_WalletModelTransaction::getPossibleKeyChange()
{
    return keyChange;
}

void Bitcredit_WalletModelTransaction::newKeyDepositSignature(Credits_CWallet *deposit_wallet)
{
    keyDepositSignature = new Credits_CReserveKey(deposit_wallet);
}

Credits_CReserveKey *Bitcredit_WalletModelTransaction::getKeyDepositSignature()
{
    return keyDepositSignature;
}

void Bitcredit_WalletModelTransaction::newKeyRecipient(Credits_CWallet *wallet)
{
	keyRecipients.push_back(new Credits_CReserveKey(wallet));
}

std::vector<Credits_CReserveKey *> & Bitcredit_WalletModelTransaction::getKeyRecipients()
{
    return keyRecipients;
}
