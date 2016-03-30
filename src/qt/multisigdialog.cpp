#include <QClipboard>
#include <QDialog>
#include <QMessageBox>
#include <QScrollBar>
#include <QKeyEvent>
#include <vector>

#include "addresstablemodel.h"
#include "base58.h"
#include "key.h"
#include "main.h"
#include "multisigaddressentry.h"
#include "multisiginputentry.h"
#include "multisigdialog.h"
#include "ui_multisigdialog.h"
#include "script.h"
#include "sendcoinsentry.h"
#include "util.h"
#include "wallet.h"
#include "walletmodel.h"

#ifdef USE_LEVELDB
#include "txdb-leveldb.h"
#else
#include "txdb-bdb.h"
#endif

MultisigDialog::MultisigDialog(QWidget *parent) : QWidget(parent), ui(new Ui::MultisigDialog), model(0)
{
    ui->setupUi(this);

#ifdef Q_WS_MAC // Icons on push buttons are very uncommon on Mac
    ui->addPubKeyButton->setIcon(QIcon());
    ui->clearButton->setIcon(QIcon());
    ui->addInputButton->setIcon(QIcon());
    ui->addOutputButton->setIcon(QIcon());
    ui->signTransactionButton->setIcon(QIcon());
    ui->sendTransactionButton->setIcon(QIcon());
#endif

    addPubKey();
    addPubKey();

    connect(ui->addPubKeyButton, SIGNAL(clicked()), this, SLOT(addPubKey()));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));

    addInput();
    addOutput();

    connect(ui->addInputButton, SIGNAL(clicked()), this, SLOT(addInput()));
    connect(ui->addOutputButton, SIGNAL(clicked()), this, SLOT(addOutput()));

    ui->signTransactionButton->setEnabled(false);
    ui->sendTransactionButton->setEnabled(false);
}

void MultisigDialog::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    if (!model)
        return;

    updateAmounts();
}

void MultisigDialog::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);

    if (!model)
        return;

    clear();
}

MultisigDialog::~MultisigDialog()
{
    delete ui;
}

void MultisigDialog::setModel(WalletModel *model)
{
    this->model = model;

    for(int i = 0; i < ui->pubkeyEntries->count(); i++)
    {
        MultisigAddressEntry *entry = qobject_cast<MultisigAddressEntry *>(ui->pubkeyEntries->itemAt(i)->widget());
        if(entry)
            entry->setModel(model);
    }


    for(int i = 0; i < ui->inputs->count(); i++)
    {
        MultisigInputEntry *entry = qobject_cast<MultisigInputEntry *>(ui->inputs->itemAt(i)->widget());
        if(entry)
            entry->setModel(model);
    }


    for(int i = 0; i < ui->outputs->count(); i++)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry *>(ui->outputs->itemAt(i)->widget());
        if(entry)
            entry->setModel(model);
    }
}

void MultisigDialog::updateRemoveEnabled()
{
    bool enabled = (ui->pubkeyEntries->count() > 2);

    for(int i = 0; i < ui->pubkeyEntries->count(); i++)
    {
        MultisigAddressEntry *entry = qobject_cast<MultisigAddressEntry *>(ui->pubkeyEntries->itemAt(i)->widget());
        if(entry)
            entry->setRemoveEnabled(enabled);
    }

    QString maxSigsStr;
    maxSigsStr.setNum(ui->pubkeyEntries->count());
    ui->maxSignaturesLabel->setText(QString("/ ") + maxSigsStr);


    enabled = (ui->inputs->count() > 1);
    for(int i = 0; i < ui->inputs->count(); i++)
    {
        MultisigInputEntry *entry = qobject_cast<MultisigInputEntry *>(ui->inputs->itemAt(i)->widget());
        if(entry)
            entry->setRemoveEnabled(enabled);
    }


    enabled = (ui->outputs->count() > 1);
    for(int i = 0; i < ui->outputs->count(); i++)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry *>(ui->outputs->itemAt(i)->widget());
        if(entry)
            entry->setRemoveEnabled(enabled);
    }
}

void MultisigDialog::on_createAddressButton_clicked()
{
    ui->multisigAddress->clear();
    ui->redeemScript->clear();

    if(!model)
        return;

    std::vector<CPubKey> pubkeys;
    pubkeys.resize(ui->pubkeyEntries->count());
    unsigned int required = ui->requiredSignatures->text().toUInt();

    for(int i = 0; i < ui->pubkeyEntries->count(); i++)
    {
        MultisigAddressEntry *entry = qobject_cast<MultisigAddressEntry *>(ui->pubkeyEntries->itemAt(i)->widget());
        if(!entry->validate())
            return;
        QString str = entry->getPubkey();
        CPubKey vchPubKey(ParseHex(str.toStdString().c_str()));
        if(!vchPubKey.IsValid())
            return;
        pubkeys[i] = vchPubKey;
    }

    if(pubkeys.size() > 16)
    {
        QMessageBox::warning(this, tr("Error"), tr("Number of addresses involved in the address creation > %1\nReduce the number").arg(16), QMessageBox::Ok);
        return;
    }

    if(required == 0)
    {
        QMessageBox::warning(this, tr("Error"), tr("Number of required signatures is 0\nNumber of required signatures must be between 1 and number of keys involved in the creation of address."), QMessageBox::Ok);
        return;
    }

    if(required > pubkeys.size())
    {
        QMessageBox::warning(this, tr("Error"), tr("Number of required signatures > Number of keys involved in the creation of address."), QMessageBox::Ok);
        return;
    }
    
    CScript script;
    script.SetMultisig(required, pubkeys);
    if (script.size() > MAX_SCRIPT_ELEMENT_SIZE)
    {
        QMessageBox::warning(this, tr("Error"), tr("Redeem script exceeds size limit: %1 > %2\nReduce the number of addresses involved in the address creation.").arg(script.size()).arg(MAX_SCRIPT_ELEMENT_SIZE), QMessageBox::Ok);
        return;
    }
    CScriptID scriptID = script.GetID();
    CBitcoinAddress address(scriptID);

    ui->multisigAddress->setText(address.ToString().c_str());
    ui->redeemScript->setText(HexStr(script.begin(), script.end()).c_str());
}

void MultisigDialog::on_copyMultisigAddressButton_clicked()
{
    QApplication::clipboard()->setText(ui->multisigAddress->text());
}

void MultisigDialog::on_copyRedeemScriptButton_clicked()
{
    QApplication::clipboard()->setText(ui->redeemScript->text());
}

void MultisigDialog::on_saveRedeemScriptButton_clicked()
{
    if(!model)
        return;

    CWallet *wallet = model->getWallet();
    std::string redeemScript = ui->redeemScript->text().toStdString();
    std::vector<unsigned char> scriptData(ParseHex(redeemScript));
    CScript script(scriptData.begin(), scriptData.end());
    CScriptID scriptID = script.GetID();

    LOCK(wallet->cs_wallet);
    if(!wallet->HaveCScript(scriptID))
        wallet->AddCScript(script);
}

void MultisigDialog::on_saveMultisigAddressButton_clicked()
{
    if(!model)
        return;

    CWallet *wallet = model->getWallet();
    std::string redeemScript = ui->redeemScript->text().toStdString();
    std::string address = ui->multisigAddress->text().toStdString();
    std::string label("multisig");

    if(!model->validateAddress(QString(address.c_str())))
        return;

    std::vector<unsigned char> scriptData(ParseHex(redeemScript));
    CScript script(scriptData.begin(), scriptData.end());
    CScriptID scriptID = script.GetID();

    LOCK(wallet->cs_wallet);
    if(!wallet->HaveCScript(scriptID))
        wallet->AddCScript(script);
    if(!wallet->mapAddressBook.count(CBitcoinAddress(address)))
        wallet->SetAddressBookName(CBitcoinAddress(address), label);
}

void MultisigDialog::clear()
{
    while(ui->pubkeyEntries->count())
        delete ui->pubkeyEntries->takeAt(0)->widget();

    addPubKey();
    addPubKey();
    updateRemoveEnabled();
}

MultisigAddressEntry * MultisigDialog::addPubKey()
{
    MultisigAddressEntry *entry = new MultisigAddressEntry(this);

    entry->setModel(model);
    ui->pubkeyEntries->addWidget(entry);
    connect(entry, SIGNAL(removeEntry(MultisigAddressEntry *)), this, SLOT(removeEntry(MultisigAddressEntry *)));
    updateRemoveEnabled();
    entry->clear();
    ui->scrollAreaWidgetContents->resize(ui->scrollAreaWidgetContents->sizeHint());
    QScrollBar *bar = ui->scrollArea->verticalScrollBar();
    if(bar)
        bar->setSliderPosition(bar->maximum());

    return entry;
}

void MultisigDialog::removeEntry(MultisigAddressEntry *entry)
{
    delete entry;
    updateRemoveEnabled();
}

void MultisigDialog::on_createTransactionButton_clicked()
{
    CTransaction transaction;

    // Get inputs
    for(int i = 0; i < ui->inputs->count(); i++)
    {
        MultisigInputEntry *entry = qobject_cast<MultisigInputEntry *>(ui->inputs->itemAt(i)->widget());
        if(entry)
        {
            if(entry->validate())
            {
                CTxIn input = entry->getInput();
                transaction.vin.push_back(input);
            }
            else
                return;
        }
    }

    // Get outputs
    for(int i = 0; i < ui->outputs->count(); i++)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry *>(ui->outputs->itemAt(i)->widget());

        if(entry)
        {
            if(entry->validate())
            {
                SendCoinsRecipient recipient = entry->getValue();
                CBitcoinAddress address(recipient.address.toStdString());
                CScript scriptPubKey;
                scriptPubKey.SetAddress(address);
                int64_t amount = recipient.amount;
                CTxOut output(amount, scriptPubKey);
                transaction.vout.push_back(output);
            }
            else
                return;
        }
    }

    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << transaction;
    ui->transaction->setText(HexStr(ss.begin(), ss.end()).c_str());
}

void MultisigDialog::on_transaction_textChanged()
{
    while(ui->inputs->count())
        delete ui->inputs->takeAt(0)->widget();
    while(ui->outputs->count())
        delete ui->outputs->takeAt(0)->widget();

    if(ui->transaction->text().size() > 0)
        ui->signTransactionButton->setEnabled(true);
    else
        ui->signTransactionButton->setEnabled(false);

    // Decode the raw transaction
    std::vector<unsigned char> txData(ParseHex(ui->transaction->text().toStdString()));
    CDataStream ss(txData, SER_NETWORK, PROTOCOL_VERSION);
    CTransaction tx;
    try
    {
        ss >> tx;
    }
    catch(const std::exception&)
    {
        return;
    }

    // Fill input list
    int index = -1;
    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    {
        uint256 prevoutHash = txin.prevout.hash;
        addInput();
        index++;
        MultisigInputEntry *entry = qobject_cast<MultisigInputEntry *>(ui->inputs->itemAt(index)->widget());
        if(entry)
        {
            entry->setTransactionId(QString(prevoutHash.GetHex().c_str()));
            entry->setTransactionOutputIndex(txin.prevout.n);
        }
    }

    // Fill output list
    index = -1;
    BOOST_FOREACH(const CTxOut& txout, tx.vout)
    {
        CScript scriptPubKey = txout.scriptPubKey;
        CTxDestination addr;
        ExtractDestination(scriptPubKey, addr);
        CBitcoinAddress address(addr);
        SendCoinsRecipient recipient;
        recipient.address = QString(address.ToString().c_str());
        recipient.amount = txout.nValue;
        addOutput();
        index++;
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry *>(ui->outputs->itemAt(index)->widget());
        if(entry)
        {
            entry->setValue(recipient);
        }
    }

    updateRemoveEnabled();
}

void MultisigDialog::on_copyTransactionButton_clicked()
{
    QApplication::clipboard()->setText(ui->transaction->text());
}

void MultisigDialog::on_pasteTransactionButton_clicked()
{
    ui->transaction->setText(QApplication::clipboard()->text());
}

void MultisigDialog::on_signTransactionButton_clicked()
{
    ui->signedTransaction->clear();

    if(!model)
        return;

    CWallet *wallet = model->getWallet();

    // Decode the raw transaction
    std::vector<unsigned char> txData(ParseHex(ui->transaction->text().toStdString()));
    CDataStream ss(txData, SER_NETWORK, PROTOCOL_VERSION);
    CTransaction tx;
    try
    {
        ss >> tx;
    }
    catch(const std::exception&)
    {
        return;
    }
    CTransaction mergedTx(tx);

    // Fetch previous transactions (inputs)
    std::map<COutPoint, CScript> mapPrevOut;
    for(unsigned int i = 0; i < mergedTx.vin.size(); i++)
    {
        CTransaction tempTx;
        MapPrevTx mapPrevTx;
        CTxDB txdb("r");
        std::map<uint256, CTxIndex> unused;
        bool fInvalid;

        tempTx.vin.push_back(mergedTx.vin[i]);
        tempTx.FetchInputs(txdb, unused, false, false, mapPrevTx, fInvalid);

        BOOST_FOREACH(const CTxIn& txin, tempTx.vin)
        {
            const uint256& prevHash = txin.prevout.hash;
            if(mapPrevTx.count(prevHash) && mapPrevTx[prevHash].second.vout.size() > txin.prevout.n)
                mapPrevOut[txin.prevout] = mapPrevTx[prevHash].second.vout[txin.prevout.n].scriptPubKey;
        }
    }

    // Add the redeem scripts to the wallet keystore
    for(int i = 0; i < ui->inputs->count(); i++)
    {
        MultisigInputEntry *entry = qobject_cast<MultisigInputEntry *>(ui->inputs->itemAt(i)->widget());
        if(entry)
        {
            QString redeemScriptStr = entry->getRedeemScript();
            if(redeemScriptStr.size() > 0)
            {
                std::vector<unsigned char> scriptData(ParseHex(redeemScriptStr.toStdString()));
                CScript redeemScript(scriptData.begin(), scriptData.end());
                wallet->AddCScript(redeemScript);
            }
        }
    }

    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
        return;

    // Sign what we can
    bool fComplete = true;
    for(unsigned int i = 0; i < mergedTx.vin.size(); i++)
    {
        CTxIn& txin = mergedTx.vin[i];
        if(mapPrevOut.count(txin.prevout) == 0)
        {
            fComplete = false;
            continue;
        }
        const CScript& prevPubKey = mapPrevOut[txin.prevout];

        txin.scriptSig.clear();
        SignSignature(*wallet, prevPubKey, mergedTx, i, SIGHASH_ALL);
        txin.scriptSig = CombineSignatures(prevPubKey, mergedTx, i, txin.scriptSig, tx.vin[i].scriptSig);
        if(!VerifyScript(txin.scriptSig, prevPubKey, mergedTx, i, true, 0))
        {
            fComplete = false;
        }
    }

    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << mergedTx;
    ui->signedTransaction->setText(HexStr(ssTx.begin(), ssTx.end()).c_str());

    if(fComplete)
    {
        ui->statusLabel->setText(tr("Transaction signature is complete"));
        ui->sendTransactionButton->setEnabled(true);
    }
    else
    {
        ui->statusLabel->setText(tr("Transaction is NOT completely signed"));
        ui->sendTransactionButton->setEnabled(false);
    }
}

void MultisigDialog::on_copySignedTransactionButton_clicked()
{
    QApplication::clipboard()->setText(ui->signedTransaction->text());
}

void MultisigDialog::on_sendTransactionButton_clicked()
{
    int64_t transactionSize = ui->signedTransaction->text().size() / 2;
    if(transactionSize == 0)
        return;

    // Check the fee
    int64_t fee = (int64_t ) (ui->fee->text().toDouble() * COIN);
    int64_t minFee = MIN_TX_FEE * (1 + (int64_t) transactionSize / 1000);
    if(fee < minFee)
    {
        QMessageBox::StandardButton ret = QMessageBox::question(this, tr("Confirm send transaction"), tr("The fee of the transaction (%1 NVC) is smaller than the expected fee (%2 NVC). Do you want to send the transaction anyway?").arg((double) fee / COIN).arg((double) minFee / COIN), QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if(ret != QMessageBox::Yes)
            return;
    }
    else if(fee > minFee)
    {
        QMessageBox::StandardButton ret = QMessageBox::question(this, tr("Confirm send transaction"), tr("The fee of the transaction (%1 NVC) is bigger than the expected fee (%2 NVC). Do you want to send the transaction anyway?").arg((double) fee / COIN).arg((double) minFee / COIN), QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if(ret != QMessageBox::Yes)
            return;
    }

    // Decode the raw transaction
    std::vector<unsigned char> txData(ParseHex(ui->signedTransaction->text().toStdString()));
    CDataStream ssData(txData, SER_NETWORK, PROTOCOL_VERSION);
    CTransaction tx;
    try
    {
        ssData >> tx;
    }
    catch(const std::exception&)
    {
        return;
    }
    uint256 txHash = tx.GetHash();

    // Check if the transaction is already in the blockchain
    CTransaction existingTx;
    uint256 blockHash = 0;
    if(GetTransaction(txHash, existingTx, blockHash))
    {
        if(blockHash != 0)
            return;
    }

    // Send the transaction to the local node
    CTxDB txdb("r");
    if(!tx.AcceptToMemoryPool(txdb, false))
    return;
    SyncWithWallets(tx, NULL, true);
    //(CInv(MSG_TX, txHash), tx);
    RelayTransaction(tx, txHash);
}

MultisigInputEntry * MultisigDialog::addInput()
{
    MultisigInputEntry *entry = new MultisigInputEntry(this);

    entry->setModel(model);
    ui->inputs->addWidget(entry);
    connect(entry, SIGNAL(removeEntry(MultisigInputEntry *)), this, SLOT(removeEntry(MultisigInputEntry *)));
    connect(entry, SIGNAL(updateAmount()), this, SLOT(updateAmounts()));
    updateRemoveEnabled();
    entry->clear();
    ui->scrollAreaWidgetContents_2->resize(ui->scrollAreaWidgetContents_2->sizeHint());
    QScrollBar *bar = ui->scrollArea_2->verticalScrollBar();
    if(bar)
        bar->setSliderPosition(bar->maximum());

    return entry;
}

void MultisigDialog::removeEntry(MultisigInputEntry *entry)
{
    delete entry;
    updateRemoveEnabled();
}

SendCoinsEntry * MultisigDialog::addOutput()
{
    SendCoinsEntry *entry = new SendCoinsEntry(this);

    entry->setModel(model);
    ui->outputs->addWidget(entry);
    connect(entry, SIGNAL(removeEntry(SendCoinsEntry *)), this, SLOT(removeEntry(SendCoinsEntry *)));
    connect(entry, SIGNAL(payAmountChanged()), this, SLOT(updateAmounts()));
    updateRemoveEnabled();
    entry->clear();
    ui->scrollAreaWidgetContents_3->resize(ui->scrollAreaWidgetContents_3->sizeHint());
    QScrollBar *bar = ui->scrollArea_3->verticalScrollBar();
    if(bar)
        bar->setSliderPosition(bar->maximum());

    return entry;
}

void MultisigDialog::removeEntry(SendCoinsEntry *entry)
{
    delete entry;
    updateRemoveEnabled();
}

void MultisigDialog::updateAmounts()
{
    // Update inputs amount
    int64_t inputsAmount = 0;
    for(int i = 0; i < ui->inputs->count(); i++)
    {
        MultisigInputEntry *entry = qobject_cast<MultisigInputEntry *>(ui->inputs->itemAt(i)->widget());
        if(entry)
            inputsAmount += entry->getAmount();
    }
    QString inputsAmountStr;
    inputsAmountStr.sprintf("%.6f", (double) inputsAmount / COIN);
    ui->inputsAmount->setText(inputsAmountStr);

    // Update outputs amount
    int64_t outputsAmount = 0;
    for(int i = 0; i < ui->outputs->count(); i++)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry *>(ui->outputs->itemAt(i)->widget());
        if(entry)
            outputsAmount += entry->getValue().amount;
    }
    QString outputsAmountStr;
    outputsAmountStr.sprintf("%.6f", (double) outputsAmount / COIN);
    ui->outputsAmount->setText(outputsAmountStr);

    // Update Fee amount
    int64_t fee = inputsAmount - outputsAmount;
    QString feeStr;
    feeStr.sprintf("%.6f", (double) fee / COIN);
    ui->fee->setText(feeStr);
}

void MultisigDialog::keyPressEvent(QKeyEvent *event)
{
#ifdef ANDROID
    if(windowType() != Qt::Widget && event->key() == Qt::Key_Back)
    {
        close();
    }
#else
    if(windowType() != Qt::Widget && event->key() == Qt::Key_Escape)
    {
        close();
    }
#endif
}
