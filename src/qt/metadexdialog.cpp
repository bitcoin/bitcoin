// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/metadexdialog.h>
#include <qt/forms/ui_metadexdialog.h>

#include <qt/omnicore_qtutils.h>

#include <qt/clientmodel.h>
#include <qt/walletmodel.h>

#include <omnicore/createpayload.h>
#include <omnicore/dbtradelist.h>
#include <omnicore/errors.h>
#include <omnicore/mdex.h>
#include <omnicore/omnicore.h>
#include <omnicore/parse_string.h>
#include <omnicore/pending.h>
#include <omnicore/rpctxobject.h>
#include <omnicore/rules.h>
#include <omnicore/sp.h>
#include <omnicore/tally.h>
#include <omnicore/uint256_extensions.h>
#include <omnicore/utilsbitcoin.h>
#include <omnicore/wallettxbuilder.h>
#include <omnicore/walletutils.h>

#include <amount.h>
#include <key_io.h>
#include <sync.h>
#include <uint256.h>
// TODO #include "wallet_ismine.h"

#include <boost/lexical_cast.hpp>
#include <boost/rational.hpp>

#include <stdint.h>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <QAbstractItemView>
#include <QDialog>
#include <QDateTime>
#include <QFont>
#include <QHeaderView>
#include <QMessageBox>
#include <QString>
#include <QTableWidgetItem>
#include <QWidget>

using std::ostringstream;

using namespace mastercore;

MetaDExDialog::MetaDExDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MetaDExDialog),
    clientModel(nullptr),
    walletModel(nullptr),
    global_metadex_market(3)
{
    ui->setupUi(this);

    ui->sellList->setColumnCount(5);
    ui->sellList->verticalHeader()->setVisible(false);
    #if QT_VERSION < 0x050000
        ui->sellList->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    #else
        ui->sellList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    #endif
    ui->sellList->setShowGrid(false);
    ui->sellList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->sellList->setSelectionMode(QAbstractItemView::NoSelection);
    ui->sellList->setAlternatingRowColors(true);
    ui->sellList->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->sellList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->sellList->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(ui->comboPairTokenA, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &MetaDExDialog::SwitchMarket);
    connect(ui->comboPairTokenB, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &MetaDExDialog::SwitchMarket);
    connect(ui->comboAddress, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &MetaDExDialog::UpdateBalance);
    connect(ui->chkTestEco, &QCheckBox::clicked, this, &MetaDExDialog::FullRefresh);
    connect(ui->buttonInvertPair, &QPushButton::clicked, this, &MetaDExDialog::InvertPair);
    connect(ui->buttonTradeHistory, &QPushButton::clicked, this, &MetaDExDialog::ShowHistory);
    connect(ui->sellAmountSaleLE, &QLineEdit::textEdited, this, &MetaDExDialog::RecalcSellValues);
    connect(ui->sellAmountDesiredLE, &QLineEdit::textEdited, this, &MetaDExDialog::RecalcSellValues);
    connect(ui->sellUnitPriceLE, &QLineEdit::textEdited, this, &MetaDExDialog::RecalcSellValues);
    connect(ui->sellList, &QTableView::doubleClicked, this, &MetaDExDialog::ShowDetails);
    connect(ui->sellButton, &QPushButton::clicked, this, &MetaDExDialog::sendTrade);

    FullRefresh();
}

MetaDExDialog::~MetaDExDialog()
{
    delete ui;
}

uint32_t MetaDExDialog::GetPropForSale()
{
    QString propStr = ui->comboPairTokenA->itemData(ui->comboPairTokenA->currentIndex()).toString();
    if (propStr.isEmpty()) return 0;
    return propStr.toUInt();
}

uint32_t MetaDExDialog::GetPropDesired()
{
    QString propStr = ui->comboPairTokenB->itemData(ui->comboPairTokenB->currentIndex()).toString();
    if (propStr.isEmpty()) return 0;
    return propStr.toUInt();
}

void MetaDExDialog::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if (nullptr != model) {
        connect(model, &ClientModel::refreshOmniState, this, &MetaDExDialog::UpdateOffers);
        connect(model, &ClientModel::refreshOmniBalance, this, &MetaDExDialog::BalanceOrderRefresh);
        connect(model, &ClientModel::reinitOmniState, this, &MetaDExDialog::FullRefresh);
    }
}

void MetaDExDialog::setWalletModel(WalletModel *model)
{
    // use wallet model to get visibility into BTC balance changes for fees
    this->walletModel = model;
}

void MetaDExDialog::PopulateAddresses()
{
    { // restrict scope of lock to address updates only (don't hold for balance update too)
        LOCK(cs_tally);

        uint32_t propertyId = GetPropForSale();
        QString currentSetAddress = ui->comboAddress->currentText();
        ui->comboAddress->clear();
        for (std::unordered_map<std::string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it) {
            std::string address = (my_it->first).c_str();
            int isMyAddress = IsMyAddress(address, &walletModel->wallet());
            uint32_t id;
            (my_it->second).init();
            while (0 != (id = (my_it->second).next())) {
                if (id == propertyId) {
                    if (!GetAvailableTokenBalance(address, propertyId)) continue; // ignore this address, has no available balance to spend
                    if (isMyAddress) ui->comboAddress->addItem((my_it->first).c_str()); // only include wallet addresses
                }
            }
        }
        int idx = ui->comboAddress->findText(currentSetAddress);
        if (idx != -1) { ui->comboAddress->setCurrentIndex(idx); }
    }
    UpdateBalance();
}

void MetaDExDialog::UpdateProperties()
{
    bool testEco = ui->chkTestEco->isChecked();

    QString currentSetPropA = ui->comboPairTokenA->currentText();
    QString currentSetPropB = ui->comboPairTokenB->currentText();
    ui->comboPairTokenA->clear();
    ui->comboPairTokenB->clear();

    for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it) {
        uint32_t propertyId = my_it->first;
        if ((testEco && !isTestEcosystemProperty(propertyId)) || (!testEco && isTestEcosystemProperty(propertyId))) continue;
        std::string spName;
        spName = getPropertyName(propertyId).c_str();
        uint32_t nameMax = 30;
        if (isTestEcosystemProperty(propertyId)) nameMax = 20;
        if (spName.size()>nameMax) {
            spName = spName.substr(0,nameMax)+"...";
        }
        std::string spId = strprintf("%d", propertyId);
        spName += " (#" + spId + ")";
        ui->comboPairTokenA->addItem(spName.c_str(),spId.c_str());
        ui->comboPairTokenB->addItem(spName.c_str(),spId.c_str());
    }

    if (ui->comboPairTokenA->count() > 1) {
        int idxA = ui->comboPairTokenA->findText(currentSetPropA);
        if (idxA != -1) {
            ui->comboPairTokenA->setCurrentIndex(idxA);
        } else {
            ui->comboPairTokenA->setCurrentIndex(0);
        }
    }
    if (ui->comboPairTokenB->count() > 2) {
        int idxB = ui->comboPairTokenB->findText(currentSetPropB);
        if (idxB != -1) {
            ui->comboPairTokenB->setCurrentIndex(idxB);
        } else {
            ui->comboPairTokenB->setCurrentIndex(1);
        }
    }
}

// Update the balance for the currently selected address
void MetaDExDialog::UpdateBalance()
{
    QString currentSetAddress = ui->comboAddress->currentText();
    if (currentSetAddress.isEmpty()) {
        ui->lblBalance->setText(QString::fromStdString("Your balance: N/A"));
        ui->lblFeeWarning->setVisible(false);
    } else {
        uint32_t propertyId = GetPropForSale();
        int64_t balanceAvailable = GetAvailableTokenBalance(currentSetAddress.toStdString(), propertyId);
        std::string sellBalStr;
        if (isPropertyDivisible(propertyId)) {
            sellBalStr = FormatDivisibleMP(balanceAvailable);
        } else {
            sellBalStr = FormatIndivisibleMP(balanceAvailable);
        }
        ui->lblBalance->setText(QString::fromStdString("Your balance: " + sellBalStr + getTokenLabel(propertyId)));
        // warning label will be lit if insufficient fees for MetaDEx payload (28 bytes)
        if (CheckFee(walletModel->wallet(), currentSetAddress.toStdString(), 28)) {
            ui->lblFeeWarning->setVisible(false);
        } else {
            ui->lblFeeWarning->setText("WARNING: The address is low on BTC for transaction fees.");
            ui->lblFeeWarning->setVisible(true);
        }
    }
}

// Change markets when one of the property selector combos is changed
void MetaDExDialog::SwitchMarket()
{
    QString propAStr = ui->comboPairTokenA->itemData(ui->comboPairTokenA->currentIndex()).toString();
    QString propBStr = ui->comboPairTokenB->itemData(ui->comboPairTokenB->currentIndex()).toString();
    if (propAStr.isEmpty() || propBStr.isEmpty()) {
        PrintToLog("QTERROR: Empty variable switching property markets.  PropA=%s, PropB=%s\n",propAStr.toStdString(),propBStr.toStdString());
        return;
    }
    uint32_t propA = propAStr.toUInt();
    uint32_t propB = propBStr.toUInt();
    if (propA == propB) {
        QMessageBox::critical( this, "Unable to change markets",
        "The property being sold and property desired cannot be the same." );
        return;
    }
    ui->sellAmountSaleLE->clear();
    ui->sellAmountDesiredLE->clear();
    ui->sellUnitPriceLE->clear();

    FullRefresh();
}

// Recalulates the form sell values when one is changed
void MetaDExDialog::RecalcSellValues()
{
    int64_t amountForSale = 0;
    int64_t amountDesired = 0;
    amountForSale = StrToInt64(ui->sellAmountSaleLE->text().toStdString(),isPropertyDivisible(GetPropForSale()));
    amountDesired = StrToInt64(ui->sellAmountDesiredLE->text().toStdString(),isPropertyDivisible(GetPropDesired()));

    if (0 >= amountForSale) {
        ui->sellAmountSaleLE->setStyleSheet(" QLineEdit { background-color:rgb(255, 204, 209); }");
    } else {
        ui->sellAmountSaleLE->setStyleSheet(" QLineEdit { }");
    }
    if (0 >= amountDesired) {
        ui->sellAmountDesiredLE->setStyleSheet(" QLineEdit { background-color:rgb(255, 204, 209); }");
    } else {
        ui->sellAmountDesiredLE->setStyleSheet(" QLineEdit { }");
    }
    if (ui->sellAmountSaleLE->text().toStdString().length()==0) ui->sellAmountSaleLE->setStyleSheet(" QLineEdit { }");
    if (ui->sellAmountDesiredLE->text().toStdString().length()==0) ui->sellAmountDesiredLE->setStyleSheet(" QLineEdit { }");

    if (0 >= amountForSale || 0>= amountDesired) {
        ui->sellUnitPriceLE->clear();
        return;
    }

    rational_t unitPrice(amountForSale, amountDesired);
    std::string unitPriceStr = xToString(unitPrice);
    if (unitPriceStr.length() > 25) unitPriceStr.resize(25); // keep the price from getting too long
    ui->sellUnitPriceLE->setText(QString::fromStdString(unitPriceStr));
}

// Performs a full refresh of all elements - for example when switching markets
void MetaDExDialog::FullRefresh()
{
    UpdateProperties();
    PopulateAddresses();
    UpdateOffers();

    ui->lblATSToken->setText(QString::fromStdString(getTokenLabel(GetPropForSale())));
    ui->lblADToken->setText(QString::fromStdString(getTokenLabel(GetPropDesired())));
    ui->sellList->setHorizontalHeaderItem(0, new QTableWidgetItem("TXID"));
    ui->sellList->setHorizontalHeaderItem(1, new QTableWidgetItem("Seller"));
    ui->sellList->setHorizontalHeaderItem(2, new QTableWidgetItem("Unit Price"));
    ui->sellList->setHorizontalHeaderItem(3, new QTableWidgetItem(QString::fromStdString(getTokenLabel(GetPropForSale())) + " For Sale"));
    ui->sellList->setHorizontalHeaderItem(4, new QTableWidgetItem(QString::fromStdString(getTokenLabel(GetPropDesired())) + " Desired"));
    ui->sellButton->setText("Sell " + QString::fromStdString(getTokenLabel(GetPropForSale())));
}

// Inverts the pair
void MetaDExDialog::InvertPair()
{
    QString propAStr = ui->comboPairTokenA->itemData(ui->comboPairTokenA->currentIndex()).toString();
    QString propBStr = ui->comboPairTokenB->itemData(ui->comboPairTokenB->currentIndex()).toString();
    QString currentSetPropA = ui->comboPairTokenA->currentText();
    QString currentSetPropB = ui->comboPairTokenB->currentText();
    if (propAStr.isEmpty() || propBStr.isEmpty()) {
        PrintToLog("QTERROR: Empty variable switching property markets.  PropA=%s, PropB=%s\n",propAStr.toStdString(),propBStr.toStdString());
        return;
    }
    uint32_t propA = propAStr.toUInt();
    uint32_t propB = propBStr.toUInt();
    uint32_t tempB = propB;
    propB = propA;
    propA = tempB;

    if (ui->comboPairTokenA->count() > 1) {
        int idxA = ui->comboPairTokenA->findText(currentSetPropB);
        if (idxA != -1) {
            ui->comboPairTokenA->setCurrentIndex(idxA);
        } else {
            ui->comboPairTokenA->setCurrentIndex(0);
        }
    }
    if (ui->comboPairTokenB->count() > 2) {
        int idxB = ui->comboPairTokenB->findText(currentSetPropA);
        if (idxB != -1) {
            ui->comboPairTokenB->setCurrentIndex(idxB);
        } else {
            ui->comboPairTokenB->setCurrentIndex(1);
        }
    }

    ui->sellAmountSaleLE->clear();
    ui->sellAmountDesiredLE->clear();
    ui->sellUnitPriceLE->clear();
    FullRefresh();
}

// Loops through the MetaDEx and updates the list of offers
void MetaDExDialog::UpdateOffers()
{
    ui->sellList->setRowCount(0);

    LOCK(cs_tally);

    // Obtain divisibility outside the loop to avoid repeatedly loading properties
    bool divisSale = isPropertyDivisible(GetPropForSale());
    bool divisDes = isPropertyDivisible(GetPropDesired());

    for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it) {
        if ((my_it->first != GetPropForSale())) continue; // not the property we're looking for, don't waste any more work
        md_PricesMap & prices = my_it->second;
        for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it) { // loop through the sell prices for the property
            std::string unitPriceStr;
            bool includesMe = false;
            md_Set & indexes = (it->second);
            for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it) { // multiple sell offers can exist at the same price, sum them for the UI
                const CMPMetaDEx& obj = *it;
                if ((obj.getDesProperty() != GetPropDesired())) continue; // not the property we're interested in
                if (IsMyAddress(obj.getAddr(), &walletModel->wallet())) includesMe = true;
                std::string strAvail;
                if (divisSale) {
                    strAvail = FormatDivisibleShortMP(obj.getAmountRemaining());
                } else {
                    strAvail = FormatIndivisibleMP(obj.getAmountRemaining());
                }
                std::string strDesired;
                if (divisDes) {
                    strDesired = FormatDivisibleShortMP(obj.getAmountToFill());
                } else {
                    strDesired = FormatIndivisibleMP(obj.getAmountToFill());
                }
                std::string priceStr = StripTrailingZeros(obj.displayFullUnitPrice());
                if (priceStr.length() > 10) {
                    priceStr.resize(10); // keep price in UI manageable,
                    priceStr += "...";
                }
                AddRow(includesMe, obj.getHash().GetHex(), obj.getAddr(), priceStr, strAvail, strDesired);
            }
        }
    }
}

// This function adds a row to the buy or sell offer list
void MetaDExDialog::AddRow(bool includesMe, const std::string& txid, const std::string& seller, const std::string& price, const std::string& available, const std::string& desired)
{
    int workingRow;
    workingRow = ui->sellList->rowCount();
    ui->sellList->insertRow(workingRow);

    QTableWidgetItem *txidCell = new QTableWidgetItem(QString::fromStdString(txid));
    QTableWidgetItem *sellerCell = new QTableWidgetItem(QString::fromStdString(seller));
    QTableWidgetItem *priceCell = new QTableWidgetItem(QString::fromStdString(price));
    QTableWidgetItem *availableCell = new QTableWidgetItem(QString::fromStdString(available));
    QTableWidgetItem *desiredCell = new QTableWidgetItem(QString::fromStdString(desired));
    if(includesMe) {
        QFont font;
        font.setBold(true);
        txidCell->setFont(font);
        sellerCell->setFont(font);
        priceCell->setFont(font);
        availableCell->setFont(font);
        desiredCell->setFont(font);
    }
    txidCell->setTextAlignment(Qt::AlignLeft + Qt::AlignVCenter);
    sellerCell->setTextAlignment(Qt::AlignLeft + Qt::AlignVCenter);
    availableCell->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
    priceCell->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
    desiredCell->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
    ui->sellList->setItem(workingRow, 0, txidCell);
    ui->sellList->setItem(workingRow, 1, sellerCell);
    ui->sellList->setItem(workingRow, 2, priceCell);
    ui->sellList->setItem(workingRow, 3, availableCell);
    ui->sellList->setItem(workingRow, 4, desiredCell);
}

// Displays details of the selected trade and any associated matches
void MetaDExDialog::ShowDetails()
{
    UniValue txobj(UniValue::VOBJ);
    uint256 txid;
    txid.SetHex(ui->sellList->item(ui->sellList->currentRow(),0)->text().toStdString());
    std::string strTXText;

    if (!txid.IsNull()) {
        // grab extended trade details via the RPC populator
        int rc = populateRPCTransactionObject(txid, txobj, "", true, "", &walletModel->wallet());
        if (rc >= 0) strTXText = txobj.write(true);
    }

    if (!strTXText.empty()) {
        PopulateSimpleDialog(strTXText, "Trade Information", "Trade Information");
    }
}

// Show trade history for this pair
void MetaDExDialog::ShowHistory()
{
    UniValue history(UniValue::VARR);
    LOCK(cs_tally);
    pDbTradeList->getTradesForPair(GetPropForSale(), GetPropDesired(), history, 50);
    std::string strHistory = history.write(true);

    if (!strHistory.empty()) {
        PopulateSimpleDialog(strHistory, "Trade History", "Trade History");
    }
}

void MetaDExDialog::BalanceOrderRefresh()
{
    PopulateAddresses();
    UpdateOffers();
}

void MetaDExDialog::sendTrade()
{
//    int blockHeight = GetHeight();

    std::string strFromAddress = ui->comboAddress->currentText().toStdString();
    int64_t amountForSale = 0;
    int64_t amountDesired = 0;
    amountForSale = StrToInt64(ui->sellAmountSaleLE->text().toStdString(),isPropertyDivisible(GetPropForSale()));
    amountDesired = StrToInt64(ui->sellAmountDesiredLE->text().toStdString(),isPropertyDivisible(GetPropDesired()));
    if (0 >= amountForSale || 0>= amountDesired) {
        QMessageBox::critical( this, "Unable to send MetaDEx trade",
        "The amount values supplied are not valid.  Please check your trade parameters and try again." );
        return;
    }

/**
    // warn if we have to truncate the amount due to a decimal amount for an indivisible property, but allow send to continue
    if (divisible) {
        size_t pos = strAmount.find(".");
        if (pos!=std::string::npos) {
            std::string tmpStrAmount = strAmount.substr(0,pos);
            std::string strMsgText = "The amount entered contains a decimal however the property being transacted is indivisible.\n\nThe amount entered will be truncated as follows:\n";
            strMsgText += "Original amount entered: " + strAmount + "\nAmount that will be used: " + tmpStrAmount + "\n\n";
            strMsgText += "Do you still wish to proceed with the transaction?";
            QString msgText = QString::fromStdString(strMsgText);
            QMessageBox::StandardButton responseClick;
            responseClick = QMessageBox::question(this, "Amount truncation warning", msgText, QMessageBox::Yes|QMessageBox::No);
            if (responseClick == QMessageBox::No) {
                QMessageBox::critical( this, "MetaDEx transaction cancelled",
                "The MetaDEx transaction has been cancelled.\n\nPlease double-check the transction details thoroughly before retrying your transaction." );
                return;
            }
            strAmount = tmpStrAmount;
            if (!sell) { ui->buyAmountLE->setText(QString::fromStdString(strAmount)); } else { ui->sellAmountLE->setText(QString::fromStdString(strAmount)); }
        }
    }
**/

    // check if sending address has enough funds
    int64_t balanceAvailable = 0;
    balanceAvailable = GetAvailableTokenBalance(strFromAddress, GetPropForSale());
    if (amountForSale > balanceAvailable) {
        QMessageBox::critical( this, "Unable to send MetaDEx transaction",
        "The selected sending address does not have a sufficient balance to cover the amount entered.\n\nPlease double-check the transction details thoroughly before retrying your transaction." );
        return;
    }

    // check if wallet is still syncing, as this will currently cause a lockup if we try to send - compare our chain to peers to see if we're up to date
    // Bitcoin Core devs have removed GetNumBlocksOfPeers, switching to a time based best guess scenario
    uint32_t intBlockDate = GetLatestBlockTime();  // uint32, not using time_t for portability
    QDateTime currentDate = QDateTime::currentDateTime();
    int secs = QDateTime::fromTime_t(intBlockDate).secsTo(currentDate);
    if(secs > 90*60) {
        QMessageBox::critical( this, "Unable to send MetaDEx transaction",
        "The client is still synchronizing.  Sending transactions can currently be performed only when the client has completed synchronizing." );
        return;
    }

    // validation checks all look ok, let's throw up a confirmation dialog
    std::string strMsgText = "You are about to send the following MetaDEx transaction, please check the details thoroughly:\n\n";
    strMsgText += "Type: Trade Request\nFrom: " + strFromAddress + "\n\n";
    strMsgText += "Sell " + FormatMP(isPropertyDivisible(GetPropForSale()), amountForSale) + getTokenLabel(GetPropForSale());
    strMsgText += " for " + FormatMP(isPropertyDivisible(GetPropDesired()), amountDesired) + getTokenLabel(GetPropDesired());
    strMsgText += "\n\nAre you sure you wish to send this transaction?";
    QString msgText = QString::fromStdString(strMsgText);
    QMessageBox::StandardButton responseClick;
    responseClick = QMessageBox::question(this, "Confirm MetaDEx transaction", msgText, QMessageBox::Yes|QMessageBox::No);
    if (responseClick == QMessageBox::No)
    {
        QMessageBox::critical( this, "MetaDEx transaction cancelled",
        "The transaction has been cancelled.\n\nPlease double-check the transction details thoroughly before retrying your transaction." );
        return;
    }

    // unlock the wallet
    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled/failed
        QMessageBox::critical( this, "MetaDEx transaction failed",
        "The transaction has been cancelled.\n\nThe wallet unlock process must be completed to send a transaction." );
        return;
    }

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_MetaDExTrade(GetPropForSale(), amountForSale, GetPropDesired(), amountDesired);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid;
    std::string rawHex;
    int result = WalletTxBuilder(strFromAddress, "", "", 0, payload, txid, rawHex, autoCommit, &walletModel->wallet());

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        std::string strError = error_str(result);
        QMessageBox::critical( this, "MetaDEx transaction failed",
        "The MetaDEx transaction has failed.\n\nThe error code was: " + QString::number(result) + "\nThe error message was:\n" + QString::fromStdString(strError));
        return;
    } else {
        if (!autoCommit) {
            PopulateSimpleDialog(rawHex, "Raw Hex (auto commit is disabled)", "Raw transaction hex");
        } else {
            PendingAdd(txid, strFromAddress, MSC_TYPE_METADEX_TRADE, GetPropForSale(), amountForSale);
            PopulateTXSentDialog(txid.GetHex());
        }
    }
}



