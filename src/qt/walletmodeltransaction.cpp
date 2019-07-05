// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "walletmodeltransaction.h"

#include "wallet/wallet.h"

WalletModelTransaction::WalletModelTransaction(const QList<SendCoinsRecipient> &_recipients) :
    recipients(_recipients),
    walletTransaction(0),
    keyChange(0),
    fee(0)
{
    walletTransaction = new CWalletTx();
}

WalletModelTransaction::~WalletModelTransaction()
{
    delete keyChange;
    delete walletTransaction;
}

QList<SendCoinsRecipient> WalletModelTransaction::getRecipients()
{
    return recipients;
}

CWalletTx *WalletModelTransaction::getTransaction()
{
    return walletTransaction;
}

unsigned int WalletModelTransaction::getTransactionSize()
{
    return (!walletTransaction ? 0 : (::GetSerializeSize(walletTransaction->tx, SER_NETWORK, PROTOCOL_VERSION)));
}

CAmount WalletModelTransaction::getTransactionFee()
{
    return fee;
}

void WalletModelTransaction::setTransactionFee(const CAmount& newFee)
{
    fee = newFee;
}

void WalletModelTransaction::reassignAmounts()
{
    // For each recipient look for a matching CTxOut in walletTransaction and reassign amounts
    for (QList<SendCoinsRecipient>::iterator it = recipients.begin(); it != recipients.end(); ++it)
    {
        SendCoinsRecipient& rcp = (*it);

        if (rcp.paymentRequest.IsInitialized())
        {
            CAmount subtotal = 0;
            const payments::PaymentDetails& details = rcp.paymentRequest.getDetails();
            for (int j = 0; j < details.outputs_size(); j++)
            {
                const payments::Output& out = details.outputs(j);
                if (out.amount() <= 0) continue;
                const unsigned char* scriptStr = (const unsigned char*)out.script().data();
                CScript scriptPubKey(scriptStr, scriptStr+out.script().size());
                for (const auto& txout : walletTransaction->tx->vout) {
                    if (txout.scriptPubKey == scriptPubKey) {
                        subtotal += txout.nValue;
                        break;
                    }
                }
            }
            rcp.amount = subtotal;
        }
        else // normal recipient (no payment request)
        {
            for (const auto& txout : walletTransaction->tx->vout) {
                CScript scriptPubKey = GetScriptForDestination(CBitcoinAddress(rcp.address.toStdString()).Get());
                if (txout.scriptPubKey == scriptPubKey) {
                    rcp.amount = txout.nValue;
                    break;
                }
            }
        }
    }
}

CAmount WalletModelTransaction::getTotalTransactionAmount()
{
    CAmount totalTransactionAmount = 0;
    for (const SendCoinsRecipient &rcp : recipients)
    {
        totalTransactionAmount += rcp.amount;
    }
    return totalTransactionAmount;
}

void WalletModelTransaction::newPossibleKeyChange(CWallet *wallet)
{
    keyChange = new CReserveKey(wallet);
}

CReserveKey *WalletModelTransaction::getPossibleKeyChange()
{
    return keyChange;
}
