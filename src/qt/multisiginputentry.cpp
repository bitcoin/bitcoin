// Copyright (c) 2015-2016 Silk Network Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QApplication>
#include <QClipboard>
#include <string>
#include <vector>

#include "base58.h"
#include "multisiginputentry.h"
#include "ui_multisiginputentry.h"
#include "main.h"
#include "script/script.h"
#include "util.h"
#include "wallet.h"
#include "walletmodel.h"
#include "chainparamsbase.h"


MultisigInputEntry::MultisigInputEntry(QWidget *parent) : QFrame(parent), ui(new Ui::MultisigInputEntry), model(0)
{
    ui->setupUi(this);
}

MultisigInputEntry::~MultisigInputEntry()
{
    delete ui;
}

void MultisigInputEntry::setModel(WalletModel *model)
{
    this->model = model;
    clear();
}

void MultisigInputEntry::clear()
{
    ui->transactionId->clear();
    ui->transactionOutput->clear();
    ui->redeemScript->clear();
}

bool MultisigInputEntry::validate()
{
    return (ui->transactionOutput->count() > 0);
}

CTxIn MultisigInputEntry::getInput()
{
    int nOutput = ui->transactionOutput->currentIndex();
    CTxIn input(COutPoint(txHash, nOutput));

    return input;
}

CAmount MultisigInputEntry::getAmount()
{
    CAmount amount = 0;
    unsigned int nOutput = ui->transactionOutput->currentIndex();
    CTransaction tx;
    uint256 blockHash = uint256S("0");

    if(GetTransaction(txHash, tx, blockHash))
    {
        if(nOutput < tx.vout.size())
        {
            const CTxOut& txOut = tx.vout[nOutput];
            amount = txOut.nValue;
        }
    }

    return amount;
}

QString MultisigInputEntry::getRedeemScript()
{
    return ui->redeemScript->text();
}

void MultisigInputEntry::setTransactionId(QString transactionId)
{
    ui->transactionId->setText(transactionId);
}

void MultisigInputEntry::setTransactionOutputIndex(int index)
{
    ui->transactionOutput->setCurrentIndex(index);
}

void MultisigInputEntry::setRemoveEnabled(bool enabled)
{
    ui->deleteButton->setEnabled(enabled);
}

void MultisigInputEntry::on_pasteTransactionIdButton_clicked()
{
    ui->transactionId->setText(QApplication::clipboard()->text());
}

void MultisigInputEntry::on_deleteButton_clicked()
{
    Q_EMIT removeEntry(this);
}

void MultisigInputEntry::on_pasteRedeemScriptButton_clicked()
{
    ui->redeemScript->setText(QApplication::clipboard()->text());
}

void MultisigInputEntry::on_transactionId_textChanged(const QString &transactionId)
{
    ui->transactionOutput->clear();
    if(transactionId.isEmpty())
        return;

    // Make list of transaction outputs
    txHash.SetHex(transactionId.toStdString().c_str());
    CTransaction tx;
    uint256 blockHash = uint256S("0");
    if(!GetTransaction(txHash, tx, blockHash))
        return;
    for(unsigned int i = 0; i < tx.vout.size(); i++)
    {
        QString idStr;
        idStr.setNum(i);
        const CTxOut& txOut = tx.vout[i];
        CAmount amount = txOut.nValue;
        QString amountStr;
        amountStr.sprintf("%.6f", (double) amount / COIN);
        CScript script = txOut.scriptPubKey;
        CTxDestination addr;
        if(ExtractDestination(script, addr))
        {
            CBitcoinAddress address(addr);
            QString addressStr(address.ToString().c_str());
            ui->transactionOutput->addItem(idStr + QString(" - ") + addressStr + QString(" - ") + amountStr + QString(" CRW"));
        }
        else
            ui->transactionOutput->addItem(idStr + QString(" - ") + amountStr + QString(" CRW"));
    }
}

void MultisigInputEntry::on_transactionOutput_currentIndexChanged(int index)
{
    if(ui->transactionOutput->itemText(index).isEmpty())
        return;

    CTransaction tx;
    uint256 blockHash = uint256S("0");
    if(!GetTransaction(txHash, tx, blockHash))
        return;
    const CTxOut& txOut = tx.vout[index];
    CScript script = txOut.scriptPubKey;

    if(script.IsPayToScriptHash())
    {
        ui->redeemScript->setEnabled(true);

        if(model)
        {
            // Try to find the redeem script
            CTxDestination dest;
            if(ExtractDestination(script, dest))
            {
                CScriptID scriptID = boost::get<CScriptID>(dest);
                CScript redeemScript;
                if(model->getWallet()->GetCScript(scriptID, redeemScript))
                {
                    std::string strRedeemScript = HexStr(redeemScript.begin(), redeemScript.end()); 
                    ui->redeemScript->setText(strRedeemScript.c_str());
                }
                else
                {
                    ui->redeemScript->clear();
                }
            }
        }
    }
    else
    {
        ui->redeemScript->setEnabled(false);
    }

    Q_EMIT updateAmount();
}
