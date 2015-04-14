// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "sendmpdialog.h"
#include "ui_sendmpdialog.h"

#include "addresstablemodel.h"
#include "bitcoinunits.h"
#include "clientmodel.h"
#include "coincontroldialog.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "wallet.h"
#include "base58.h"
#include "coincontrol.h"
#include "ui_interface.h"
#include "walletmodel.h"

#include <boost/filesystem.hpp>

#include "leveldb/db.h"
#include "leveldb/write_batch.h"

// potentially overzealous includes here
#include "base58.h"
#include "rpcserver.h"
#include "init.h"
#include "util.h"
#include <fstream>
#include <algorithm>
#include <vector>
#include <utility>
#include <string>
#include <boost/assign/list_of.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "leveldb/db.h"
#include "leveldb/write_batch.h"
// end potentially overzealous includes
using namespace json_spirit; // since now using Array in mastercore.h this needs to come first

#include "mastercore.h"
using namespace mastercore;

// potentially overzealous using here
using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace leveldb;
// end potentially overzealous using

#include "mastercore_dex.h"
#include "mastercore_parse_string.h"
#include "mastercore_tx.h"
#include "mastercore_sp.h"
#include "mastercore_errors.h"
#include "omnicore_qtutils.h"

#include <QDateTime>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextDocument>

SendMPDialog::SendMPDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SendMPDialog),
    clientModel(0),
    walletModel(0)
{
    ui->setupUi(this);
//    this->model = model;

#ifdef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    ui->clearButton->setIcon(QIcon());
    ui->sendButton->setIcon(QIcon());
#endif
#if QT_VERSION >= 0x040700
    // populate placeholder text
    ui->sendToLineEdit->setPlaceholderText("Enter a Omni Layer address (e.g. 1oMn1LaYeRADDreSShef77z6A5S4P)");
    ui->amountLineEdit->setPlaceholderText("Enter Amount");
#endif

    // connect actions
    connect(ui->propertyComboBox, SIGNAL(activated(int)), this, SLOT(propertyComboBoxChanged(int)));
    connect(ui->sendFromComboBox, SIGNAL(activated(int)), this, SLOT(sendFromComboBoxChanged(int)));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clearButtonClicked()));
    connect(ui->sendButton, SIGNAL(clicked()), this, SLOT(sendButtonClicked()));

    // initial update
    balancesUpdated();
}

void SendMPDialog::setClientModel(ClientModel *model)
{
    if (model != NULL) {
        this->clientModel = model;
        connect(model, SIGNAL(refreshOmniState()), this, SLOT(balancesUpdated()));
    }
}

void SendMPDialog::setWalletModel(WalletModel *model)
{
    if (model != NULL) {
        this->walletModel = model;
        connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(balancesUpdated()));
    }
}

void SendMPDialog::updatePropSelector()
{
    QString spId = ui->propertyComboBox->itemData(ui->propertyComboBox->currentIndex()).toString();
    ui->propertyComboBox->clear();
    for (unsigned int propertyId = 1; propertyId<100000; propertyId++)
    {
        if ((global_balance_money_maineco[propertyId] > 0) || (global_balance_reserved_maineco[propertyId] > 0))
        {
            string spName;
            spName = getPropertyName(propertyId).c_str();
            if(spName.size()>20) spName=spName.substr(0,23)+"...";
            string spId = static_cast<ostringstream*>( &(ostringstream() << propertyId) )->str();
            spName += " (#" + spId + ")";
            if (isPropertyDivisible(propertyId)) { spName += " [D]"; } else { spName += " [I]"; }
            ui->propertyComboBox->addItem(spName.c_str(),spId.c_str());
        }
    }
    for (unsigned int propertyId = 1; propertyId<100000; propertyId++)
    {
        if ((global_balance_money_testeco[propertyId] > 0) || (global_balance_reserved_testeco[propertyId] > 0))
        {
            string spName;
            spName = getPropertyName(propertyId+2147483647).c_str();
            if(spName.size()>20) spName=spName.substr(0,23)+"...";
            string spId = static_cast<ostringstream*>( &(ostringstream() << propertyId+2147483647) )->str();
            spName += " (#" + spId + ")";
            if (isPropertyDivisible(propertyId+2147483647)) { spName += " [D]"; } else { spName += " [I]"; }
            ui->propertyComboBox->addItem(spName.c_str(),spId.c_str());
        }
    }
    int propIdx = ui->propertyComboBox->findData(spId);
    if (propIdx != -1) { ui->propertyComboBox->setCurrentIndex(propIdx); }
}

void SendMPDialog::clearFields()
{
    ui->sendToLineEdit->setText("");
    ui->amountLineEdit->setText("");
}

void SendMPDialog::updateFrom()
{
    // check if this from address has sufficient fees for a send, if not light up warning label
    std::string currentSetFromAddress = ui->sendFromComboBox->currentText().toStdString();
    size_t spacer = currentSetFromAddress.find(" ");
    if (spacer!=std::string::npos) {
        currentSetFromAddress = currentSetFromAddress.substr(0,spacer);
        ui->sendFromComboBox->setEditable(true);
        QLineEdit *comboDisplay = ui->sendFromComboBox->lineEdit();
        comboDisplay->setText(QString::fromStdString(currentSetFromAddress));
        comboDisplay->setReadOnly(true);
    } else {
        currentSetFromAddress = "";
    }
    int64_t inputTotal = feeCheck(currentSetFromAddress);
    //int64_t minWarn = 3 * CTransaction::nMinRelayTxFee + 2 * nTransactionFee;
    int64_t minWarn = 20000; //temp
    // warn when fees are not sufficient to cover a 2 KB simple send
    if (inputTotal >= minWarn)
    {
        ui->feeWarningLabel->setVisible(false);
    }
    else
    {
        std::string feeWarning = "WARNING: The sending address is low on BTC for transaction fees. "
                     "Please topup the BTC balance for the sending address to send Omni Layer transactions.";
        ui->feeWarningLabel->setText(QString::fromStdString(feeWarning));
        ui->feeWarningLabel->setVisible(true);
    }
    QString spId = ui->propertyComboBox->itemData(ui->propertyComboBox->currentIndex()).toString();
    unsigned int propertyId = spId.toUInt();
    if (propertyId > 0) {
        std::string tokenLabel;
        if (propertyId==1) tokenLabel = " MSC";
        if (propertyId==2) tokenLabel = " TMSC";
        if (propertyId>2) tokenLabel = " SPT";
        int64_t balanceAvailable = getUserAvailableMPbalance(currentSetFromAddress, propertyId);
        if (isPropertyDivisible(propertyId)) {
            ui->balanceLabel->setText(QString::fromStdString("Address Balance: " + FormatDivisibleMP(balanceAvailable) + tokenLabel));
        } else {
            ui->balanceLabel->setText(QString::fromStdString("Address Balance: " + FormatIndivisibleMP(balanceAvailable) + tokenLabel));
        }
    }
}

void SendMPDialog::updateProperty()
{
    // get currently selected from address
    std::string currentSetFromAddress = ui->sendFromComboBox->currentText().toStdString();

    // clear address selector
    ui->sendFromComboBox->clear();

    // populate from address selector
    QString spId = ui->propertyComboBox->itemData(ui->propertyComboBox->currentIndex()).toString();
    unsigned int propertyId = spId.toUInt();
    bool divisible = isPropertyDivisible(propertyId);
    std::string tokenLabel;
    if (propertyId==1) tokenLabel = " MSC";
    if (propertyId==2) tokenLabel = " TMSC";
    if (propertyId>2) tokenLabel = " SPT";
    LOCK(cs_tally);
    for(map<string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it)
    {
        string address = (my_it->first).c_str();
        string balance = "";
        unsigned int id;
        bool includeAddress=false;
        (my_it->second).init();
        while (0 != (id = (my_it->second).next()))
        {
            if(id==propertyId) { includeAddress=true; break; }
        }
        if (!includeAddress) continue; //ignore this address, has never transacted in this propertyId
        if (!IsMyAddress(address)) continue; //ignore this address, it's not ours
        if ((address.substr(0,1)=="2") || (address.substr(0,3)=="3")) continue; //quick hack to not show P2SH addresses in from selector (can't be sent from UI)
        int64_t balanceAvailable = getUserAvailableMPbalance(address, propertyId);
        if (divisible)
        {
          balance = " \t" + FormatDivisibleMP(balanceAvailable) + tokenLabel;
        }
        else {
          balance = " \t" + FormatIndivisibleMP(balanceAvailable) + tokenLabel;
        }
        ui->sendFromComboBox->addItem(((my_it->first)+balance).c_str());
    }

    // attempt to set from address back to what was originally in there before update
    int fromIdx = ui->sendFromComboBox->findText(QString::fromStdString(currentSetFromAddress), Qt::MatchContains);
    if (fromIdx != -1) { ui->sendFromComboBox->setCurrentIndex(fromIdx); } // -1 means the currently set from address doesn't have a balance in the newly selected property

    // populate balance for global wallet
    int64_t globalAvailable = 0;
    if (propertyId<2147483648) { globalAvailable = global_balance_money_maineco[propertyId]; } else { globalAvailable = global_balance_money_testeco[propertyId-2147483647]; }
    QString globalLabel;
    if (divisible)
    {
        globalLabel = QString::fromStdString("Wallet Balance (Available): " + FormatDivisibleMP(globalAvailable) + tokenLabel);
    }
    else
    {
        globalLabel = QString::fromStdString("Wallet Balance (Available): " + FormatIndivisibleMP(globalAvailable) + tokenLabel);
    }
    ui->globalBalanceLabel->setText(globalLabel);

#if QT_VERSION >= 0x040700
    // update placeholder text
    if (isPropertyDivisible(propertyId)) {
        ui->amountLineEdit->setPlaceholderText("Enter Divisible Amount");
    } else {
        ui->amountLineEdit->setPlaceholderText("Enter Indivisible Amount");
    }
#endif
}

void SendMPDialog::sendMPTransaction()
{
    // get the property being sent and get divisibility
    QString spId = ui->propertyComboBox->itemData(ui->propertyComboBox->currentIndex()).toString();
    if (spId.toStdString().empty())
    {
        QMessageBox::critical( this, "Unable to send transaction",
        "The property selected is not valid.\n\nPlease double-check the transction details thoroughly before retrying your send transaction." );
        return;
    }
    unsigned int propertyId = spId.toUInt();
    bool divisible = isPropertyDivisible(propertyId);

    // obtain the selected sender address
    string strFromAddress = ui->sendFromComboBox->currentText().toStdString();

    // push recipient address into a CBitcoinAddress type and check validity
    CBitcoinAddress fromAddress;
    if (false == strFromAddress.empty()) { fromAddress.SetString(strFromAddress); }
    if (!fromAddress.IsValid())
    {
        QMessageBox::critical( this, "Unable to send transaction",
        "The sender address selected is not valid.\n\nPlease double-check the transction details thoroughly before retrying your send transaction." );
        return;
    }

    // obtain the entered recipient address
    string strRefAddress = ui->sendToLineEdit->text().toStdString();
    // push recipient address into a CBitcoinAddress type and check validity
    CBitcoinAddress refAddress;
    if (false == strRefAddress.empty()) { refAddress.SetString(strRefAddress); }
    if (!refAddress.IsValid())
    {
        QMessageBox::critical( this, "Unable to send transaction",
        "The recipient address entered is not valid.\n\nPlease double-check the transction details thoroughly before retrying your send transaction." );
        return;
    }

    // warn if we have to truncate the amount due to a decimal amount for an indivisible property, but allow send to continue
    string strAmount = ui->amountLineEdit->text().toStdString();
    if (!divisible)
    {
        size_t pos = strAmount.find(".");
        if (pos!=std::string::npos)
        {
            string tmpStrAmount = strAmount.substr(0,pos);
            string strMsgText = "The amount entered contains a decimal however the property being sent is indivisible.\n\nThe amount entered will be truncated as follows:\n";
            strMsgText += "Original amount entered: " + strAmount + "\nAmount that will be sent: " + tmpStrAmount + "\n\n";
            strMsgText += "Do you still wish to proceed with the transaction?";
            QString msgText = QString::fromStdString(strMsgText);
            QMessageBox::StandardButton responseClick;
            responseClick = QMessageBox::question(this, "Amount truncation warning", msgText, QMessageBox::Yes|QMessageBox::No);
            if (responseClick == QMessageBox::No)
            {
                QMessageBox::critical( this, "Send transaction cancelled",
                "The send transaction has been cancelled.\n\nPlease double-check the transction details thoroughly before retrying your send transaction." );
                return;
            }
            strAmount = tmpStrAmount;
            ui->amountLineEdit->setText(QString::fromStdString(strAmount));
        }
    }

    // use strToInt64 function to get the amount, using divisibility of the property
    int64_t sendAmount = StrToInt64(strAmount, divisible);
    if (0>=sendAmount)
    {
        QMessageBox::critical( this, "Unable to send transaction",
        "The amount entered is not valid.\n\nPlease double-check the transction details thoroughly before retrying your send transaction." );
        return;
    }

    // check if sending address has enough funds
    int64_t balanceAvailable = getUserAvailableMPbalance(fromAddress.ToString(), propertyId); //getMPbalance(fromAddress.ToString(), propertyId, MONEY);
    if (sendAmount>balanceAvailable)
    {
        QMessageBox::critical( this, "Unable to send transaction",
        "The selected sending address does not have a sufficient balance to cover the amount entered.\n\nPlease double-check the transction details thoroughly before retrying your send transaction." );
        return;
    }

    // check if wallet is still syncing, as this will currently cause a lockup if we try to send - compare our chain to peers to see if we're up to date
    // Bitcoin Core devs have removed GetNumBlocksOfPeers, switching to a time based best guess scenario
    uint32_t intBlockDate = GetLatestBlockTime();  // uint32, not using time_t for portability
    QDateTime currentDate = QDateTime::currentDateTime();
    int secs = QDateTime::fromTime_t(intBlockDate).secsTo(currentDate);
    if(secs > 90*60)
    {
        QMessageBox::critical( this, "Unable to send transaction",
        "The client is still synchronizing.  Sending transactions can currently be performed only when the client has completed synchronizing." );
        return;
    }

    // validation checks all look ok, let's throw up a confirmation dialog
    string strMsgText = "You are about to send the following transaction, please check the details thoroughly:\n\n";
    string propDetails = getPropertyName(propertyId).c_str();
    string spNum = static_cast<ostringstream*>( &(ostringstream() << propertyId) )->str();
    propDetails += " (#" + spNum + ")";
    strMsgText += "From: " + fromAddress.ToString() + "\nTo: " + refAddress.ToString() + "\nProperty: " + propDetails + "\nAmount that will be sent: ";
    if (divisible) { strMsgText += FormatDivisibleMP(sendAmount); } else { strMsgText += FormatIndivisibleMP(sendAmount); }
    strMsgText += "\n\nAre you sure you wish to send this transaction?";
    QString msgText = QString::fromStdString(strMsgText);
    QMessageBox::StandardButton responseClick;
    responseClick = QMessageBox::question(this, "Confirm send transaction", msgText, QMessageBox::Yes|QMessageBox::No);
    if (responseClick == QMessageBox::No)
    {
        QMessageBox::critical( this, "Send transaction cancelled",
        "The send transaction has been cancelled.\n\nPlease double-check the transction details thoroughly before retrying your send transaction." );
        return;
    }

    // unlock the wallet
    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled/failed
        return;
    }

    // create a payload for the transaction
    // #CLASSC# std::vector<unsigned char> payload = CreatePayload_SimpleSend(propertyId, sendAmount);

    // request the wallet build the transaction (and if needed commit it) - note UI does not support added reference amounts currently
    uint256 txid = 0;
    std::string rawHex;
    int result = 0; // #CLASSC# int result = ClassAgnosticWalletTXBuilder(fromAddress.ToString(), refAddress.ToString(), "", 0, payload, txid, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        string strError = error_str(result);
        QMessageBox::critical( this, "Send transaction failed",
        "The send transaction has been cancelled.\n\nThe wallet unlock process must be completed to send a transaction." );
        return;
    } else {
        if (0) { // #CLASSC# if (!autoCommit) {
            PopulateSimpleDialog(rawHex, "Raw Hex (auto commit is disabled)", "Raw transaction hex");
        } else {
            PopulateTXSentDialog(txid.GetHex());
            // TODO PENDING OBJECT
        }
    }
    clearFields();
}

void SendMPDialog::sendFromComboBoxChanged(int idx)
{
    updateFrom();
}

void SendMPDialog::propertyComboBoxChanged(int idx)
{
    updateProperty();
    updateFrom();
}

void SendMPDialog::clearButtonClicked()
{
    clearFields();
}

void SendMPDialog::sendButtonClicked()
{
    sendMPTransaction();
}

void SendMPDialog::balancesUpdated()
{
    set_wallet_totals();

    updatePropSelector();
    updateProperty();
    updateFrom();
}
