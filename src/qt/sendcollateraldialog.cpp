// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "sendcollateraldialog.h"

#include "sendcoinsdialog.h"
#include "bitcoinunits.h"
#include "optionsmodel.h"

SendCollateralDialog::SendCollateralDialog(Node node, QWidget *parent) :
    SendCoinsDialog(parent),
    fAutoCreate(false),
    node(node)
{
}

void SendCollateralDialog::send(QList<SendCoinsRecipient> &recipients)
{
    QStringList formatted = constructConfirmationMessage(recipients);
    fAutoCreate = true;
    checkAndSend(recipients, formatted);
    fAutoCreate = false;
}

bool SendCollateralDialog::instantXChecked()
{
    return false;
}

void SendCollateralDialog::processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
{
    QPair<QString, CClientUIInterface::MessageBoxFlags> msgParams;
    msgParams.second = CClientUIInterface::MSG_WARNING;
    int collateral = SYSTEMNODE_COLLATERAL;
    QString nodeType = "Systemnode";
    if (node == MASTERNODE) {
        collateral = MASTERNODE_COLLATERAL;
        nodeType = "Masternode";
    }
    QString formattedAmount = BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), collateral * COIN);

    switch(sendCoinsReturn.status)
    {
        case WalletModel::AmountExceedsBalance:
            msgParams.first = tr("Your %1 has not been generated at this time due to not having enough Crown coins.").arg(nodeType);
            msgParams.first += QString("<br>") + tr("For a %1 you will require %2").arg(nodeType).arg(formattedAmount);
            break;
        case WalletModel::AmountWithFeeExceedsBalance:
            msgParams.first = tr("The total exceeds your balance when the %1 transaction fee is included.").arg(msgArg);
            msgParams.first += QString("<br>") + tr("Your %1 has not been generated at this time due to not having enough Crown coins.").arg(nodeType);
            msgParams.first += QString("<br>") + tr("For a %1 you will require %2 + fee").arg(nodeType).arg(formattedAmount);
            break;
        default:
            // Here handled only the case when amount exceeds balance. Other errors handled in base.
            SendCoinsDialog::processSendCoinsReturn(sendCoinsReturn, msgArg);
            return;
    }

    emit message(tr("Send Coins"), msgParams.first, msgParams.second);
}
