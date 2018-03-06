// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/walletmodeltransaction.h>

#include <interfaces/node.h>
#include <key_io.h>

WalletModelTransaction::WalletModelTransaction(const QList<SendCoinsRecipient> &_recipients) :
    recipients(_recipients),
    fee(0)
{
}

QList<SendCoinsRecipient> WalletModelTransaction::getRecipients() const
{
    return recipients;
}

std::unique_ptr<interfaces::PendingWalletTx>& WalletModelTransaction::getWtx()
{
    return wtx;
}

unsigned int WalletModelTransaction::getTransactionSize()
{
    return wtx != nullptr ? ::GetSerializeSize(wtx->get(), SER_NETWORK, PROTOCOL_VERSION) : 0;
}

CAmount WalletModelTransaction::getTransactionFee() const
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
                for (const auto& txout : wtx->get().vout) {
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
            for (const auto& txout : wtx->get().vout) {
                CScript scriptPubKey = GetScriptForDestination(DecodeDestination(rcp.address.toStdString()));
                if (txout.scriptPubKey == scriptPubKey) {
                    rcp.amount = txout.nValue;
                    break;
                }
            }
        }
    }
}

CAmount WalletModelTransaction::getTotalTransactionAmount() const
{
    CAmount totalTransactionAmount = 0;
    for (const SendCoinsRecipient &rcp : recipients)
    {
        totalTransactionAmount += rcp.amount;
    }
    return totalTransactionAmount;
}
