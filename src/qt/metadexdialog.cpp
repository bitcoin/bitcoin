// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "metadexdialog.h"
#include "ui_metadexdialog.h"

#include "omnicore_qtutils.h"

#include "clientmodel.h"
#include "walletmodel.h"

#include "omnicore/createpayload.h"
#include "omnicore/errors.h"
#include "omnicore/mdex.h"
#include "omnicore/omnicore.h"
#include "omnicore/parse_string.h"
#include "omnicore/pending.h"
#include "omnicore/rules.h"
#include "omnicore/sp.h"
#include "omnicore/tally.h"
#include "omnicore/utilsbitcoin.h"

#include "amount.h"
#include "sync.h"
#include "uint256.h"
#include "wallet_ismine.h"

#include <boost/lexical_cast.hpp>

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
using std::string;

using namespace mastercore;

MetaDExDialog::MetaDExDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MetaDExDialog),
    clientModel(0),
    walletModel(0),
    global_metadex_market(3)
{
    ui->setupUi(this);

    //prep lists
    ui->buyList->setColumnCount(3);
    ui->sellList->setColumnCount(3);
    ui->buyList->verticalHeader()->setVisible(false);
    #if QT_VERSION < 0x050000
        ui->buyList->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    #else
        ui->buyList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    #endif
    ui->buyList->setShowGrid(false);
    ui->buyList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->buyList->setSelectionMode(QAbstractItemView::NoSelection);
    ui->buyList->setFocusPolicy(Qt::NoFocus);
    ui->buyList->setAlternatingRowColors(true);
    ui->sellList->verticalHeader()->setVisible(false);
    #if QT_VERSION < 0x050000
        ui->sellList->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    #else
        ui->sellList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    #endif
    ui->sellList->setShowGrid(false);
    ui->sellList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->sellList->setSelectionMode(QAbstractItemView::NoSelection);
    ui->sellList->setFocusPolicy(Qt::NoFocus);
    ui->sellList->setAlternatingRowColors(true);

    connect(ui->switchButton, SIGNAL(clicked()), this, SLOT(switchButtonClicked()));
    connect(ui->buyButton, SIGNAL(clicked()), this, SLOT(buyTrade()));
    connect(ui->sellButton, SIGNAL(clicked()), this, SLOT(sellTrade()));
    connect(ui->sellAddressCombo, SIGNAL(activated(int)), this, SLOT(sellAddressComboBoxChanged(int)));
    connect(ui->buyAddressCombo, SIGNAL(activated(int)), this, SLOT(buyAddressComboBoxChanged(int)));
    connect(ui->sellAmountLE, SIGNAL(textEdited(const QString &)), this, SLOT(recalcSellTotal()));
    connect(ui->sellPriceLE, SIGNAL(textEdited(const QString &)), this, SLOT(recalcSellTotal()));
    connect(ui->buyAmountLE, SIGNAL(textEdited(const QString &)), this, SLOT(recalcBuyTotal()));
    connect(ui->buyPriceLE, SIGNAL(textEdited(const QString &)), this, SLOT(recalcBuyTotal()));
    connect(ui->sellList, SIGNAL(cellClicked(int,int)), this, SLOT(sellClicked(int,int)));
    connect(ui->buyList, SIGNAL(cellClicked(int,int)), this, SLOT(buyClicked(int,int)));

    FullRefresh();

}

MetaDExDialog::~MetaDExDialog()
{
    delete ui;
}

void MetaDExDialog::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if (NULL != model) {
        connect(model, SIGNAL(refreshOmniState()), this, SLOT(OrderRefresh()));
        connect(model, SIGNAL(refreshOmniBalance()), this, SLOT(BalanceOrderRefresh()));
        connect(model, SIGNAL(reinitOmniState()), this, SLOT(FullRefresh()));
    }
}

void MetaDExDialog::setWalletModel(WalletModel *model)
{
    // use wallet model to get visibility into BTC balance changes for fees
    this->walletModel = model;
    if (model != NULL) {
       connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(UpdateBalances()));
    }
}

void MetaDExDialog::PopulateAddresses()
{
    { // restrict scope of lock to address updates only (don't hold for balance update too)
        LOCK(cs_tally);

        uint32_t propertyId = global_metadex_market;
        bool testeco = false;
        if (propertyId >= TEST_ECO_PROPERTY_1) testeco = true;

        // get currently selected addresses
        QString currentSetBuyAddress = ui->buyAddressCombo->currentText();
        QString currentSetSellAddress = ui->sellAddressCombo->currentText();

        // clear address selectors
        ui->buyAddressCombo->clear();
        ui->sellAddressCombo->clear();

        // populate buy and sell addresses
        for (std::map<string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it) {
            string address = (my_it->first).c_str();
            unsigned int id;
            (my_it->second).init();
            while (0 != (id = (my_it->second).next())) {
                if (id == propertyId) {
                    if (!getUserAvailableMPbalance(address, propertyId)) continue; // ignore this address, has no available balance to spend
                    if (IsMyAddress(address) == ISMINE_SPENDABLE) ui->sellAddressCombo->addItem((my_it->first).c_str()); // only include wallet addresses
                }
                if (id == OMNI_PROPERTY_MSC && !testeco) {
                    if (!getUserAvailableMPbalance(address, OMNI_PROPERTY_MSC)) continue;
                    if (IsMyAddress(address) == ISMINE_SPENDABLE) ui->buyAddressCombo->addItem((my_it->first).c_str());
                }
                if (id == OMNI_PROPERTY_TMSC && testeco) {
                    if (!getUserAvailableMPbalance(address, OMNI_PROPERTY_TMSC)) continue;
                    if (IsMyAddress(address) == ISMINE_SPENDABLE) ui->buyAddressCombo->addItem((my_it->first).c_str());
                }
            }
        }

        // attempt to set buy and sell addresses back to values before refresh - may not be possible if it's a new market
        int sellIdx = ui->sellAddressCombo->findText(currentSetSellAddress);
        if (sellIdx != -1) { ui->sellAddressCombo->setCurrentIndex(sellIdx); }
        int buyIdx = ui->buyAddressCombo->findText(currentSetBuyAddress);
        if (buyIdx != -1) { ui->buyAddressCombo->setCurrentIndex(buyIdx); }
    }

    // update balances
    UpdateBalances();
}

void MetaDExDialog::BalanceOrderRefresh()
{
    // balances have been updated, there may be a new address
    PopulateAddresses();
    // refresh orders
    OrderRefresh();
}

void MetaDExDialog::OrderRefresh()
{
    UpdateOffers();
}

// Executed when the switch market button is clicked
void MetaDExDialog::SwitchMarket()
{
    // perform some checks on the input data before attempting to switch market
    int64_t searchPropertyId = 0;
    string searchText = ui->switchLineEdit->text().toStdString();
    // check for empty field
    if (searchText.empty()) {
        QMessageBox::critical( this, "Unable to switch market",
        "You must enter a property ID to switch the market." );
        return;
    }
    // check that we can cast the field to numerical OK
    try {
        searchPropertyId = boost::lexical_cast<int64_t>(searchText);
    } catch(const boost::bad_lexical_cast &e) { // error casting, searchText likely not numerical
        QMessageBox::critical( this, "Unable to switch market",
        "The property ID entered was not a valid number." );
        return;
    }
    // check that the value is in range
    if ((searchPropertyId < 0) || (searchPropertyId > 4294967295L)) {
        QMessageBox::critical( this, "Unable to switch market",
        "The property ID entered is outside the allowed range." );
        return;
    }
    // check that not trying to trade primary for primary (eg market 1 or 2)
    if ((searchPropertyId == 1) || (searchPropertyId == 2)) {
        QMessageBox::critical( this, "Unable to switch market",
        "You cannot trade MSC/TMSC against itself." );
        return;
    }
    // check that the property exists
    bool spExists = false;
    {
        LOCK(cs_tally);
        spExists = _my_sps->hasSP(searchPropertyId);
    }
    if (!spExists) {
        QMessageBox::critical( this, "Unable to switch market",
        "The property ID entered was not found." );
        return;
    }

    // with checks complete change the market to the entered property ID and perform a full refresh
    global_metadex_market = searchPropertyId;
    ui->buyAmountLE->clear();
    ui->buyPriceLE->clear();
    ui->sellAmountLE->clear();
    ui->sellPriceLE->clear();
    FullRefresh();
}

/**
 * When a row on the buy side is clicked, populate the sell fields to allow easy selling into the offer accordingly
 */
void MetaDExDialog::buyClicked(int row, int col)
{
    // Populate the sell price field with the price clicked
    QTableWidgetItem* priceCell = ui->buyList->item(row,2);
    ui->sellPriceLE->setText(priceCell->text());

    // If the cheapest price is not chosen, make the assumption that user wants to sell down to that price point
    if (row != 0) {
        int64_t totalAmount = 0;
        bool divisible = isPropertyDivisible(global_metadex_market);
        for (int i = 0; i <= row; i++) {
            QTableWidgetItem* amountCell = ui->buyList->item(i,1);
            int64_t amount = StrToInt64(amountCell->text().toStdString(), divisible);
            totalAmount += amount;
        }
        if (divisible) {
            ui->sellAmountLE->setText(QString::fromStdString(FormatDivisibleShortMP(totalAmount)));
        } else {
            ui->sellAmountLE->setText(QString::fromStdString(FormatIndivisibleMP(totalAmount)));
        }
    } else {
        QTableWidgetItem* amountCell = ui->buyList->item(row,1);
        ui->sellAmountLE->setText(amountCell->text());
    }

    // Update the total
    recalcTotal(false);
}

/**
 * When a row on the sell side is clicked, populate the buy fields to allow easy buying into the offer accordingly
 */
void MetaDExDialog::sellClicked(int row, int col)
{
    // Populate the buy price field with the price clicked
    QTableWidgetItem* priceCell = ui->sellList->item(row,0);
    ui->buyPriceLE->setText(priceCell->text());

    // If the cheapest price is not chosen, make the assumption that user wants to buy all available up to that price point
    if (row != 0) {
        int64_t totalAmount = 0;
        bool divisible = isPropertyDivisible(global_metadex_market);
        for (int i = 0; i <= row; i++) {
            QTableWidgetItem* amountCell = ui->sellList->item(i,1);
            int64_t amount = StrToInt64(amountCell->text().toStdString(), divisible);
            totalAmount += amount;
        }
        if (divisible) {
            ui->buyAmountLE->setText(QString::fromStdString(FormatDivisibleShortMP(totalAmount)));
        } else {
            ui->buyAmountLE->setText(QString::fromStdString(FormatIndivisibleMP(totalAmount)));
        }
    } else {
        QTableWidgetItem* amountCell = ui->sellList->item(row,1);
        ui->buyAmountLE->setText(amountCell->text());
    }

    // Update the total
    recalcTotal(true);
}

// This function adds a row to the buy or sell offer list
void MetaDExDialog::AddRow(bool useBuyList, bool includesMe, const string& price, const string& available, const string& total)
{
    int workingRow;
    if (useBuyList) {
        workingRow = ui->buyList->rowCount();
        ui->buyList->insertRow(workingRow);
    } else {
        workingRow = ui->sellList->rowCount();
        ui->sellList->insertRow(workingRow);
    }
    QTableWidgetItem *priceCell = new QTableWidgetItem(QString::fromStdString(price));
    QTableWidgetItem *availableCell = new QTableWidgetItem(QString::fromStdString(available));
    QTableWidgetItem *totalCell = new QTableWidgetItem(QString::fromStdString(total));
    if(includesMe) {
        QFont font;
        font.setBold(true);
        priceCell->setFont(font);
        availableCell->setFont(font);
        totalCell->setFont(font);
    }
    if (useBuyList) {
        priceCell->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
        availableCell->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
        totalCell->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
        ui->buyList->setItem(workingRow, 0, totalCell);
        ui->buyList->setItem(workingRow, 1, availableCell);
        ui->buyList->setItem(workingRow, 2, priceCell);
    } else {
        priceCell->setTextAlignment(Qt::AlignLeft + Qt::AlignVCenter);
        availableCell->setTextAlignment(Qt::AlignLeft + Qt::AlignVCenter);
        totalCell->setTextAlignment(Qt::AlignLeft + Qt::AlignVCenter);
        ui->sellList->setItem(workingRow, 0, priceCell);
        ui->sellList->setItem(workingRow, 1, availableCell);
        ui->sellList->setItem(workingRow, 2, totalCell);
    }
}

void MetaDExDialog::UpdateBalances()
{
    // update the balances for the buy and sell addreses
    UpdateBuyAddressBalance();
    UpdateSellAddressBalance();
}

// This function loops through the MetaDEx and updates the list of buy/sell offers
void MetaDExDialog::UpdateOffers()
{
    for (int useBuyList = 0; useBuyList < 2; ++useBuyList) {
        if (useBuyList) { ui->buyList->setRowCount(0); } else { ui->sellList->setRowCount(0); }
        bool testeco = isTestEcosystemProperty(global_metadex_market);

        LOCK(cs_tally);

        for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it) {
            if ((!useBuyList) && (my_it->first != global_metadex_market)) { continue; } // not the property we're looking for, don't waste any more work
            if ((useBuyList) && (((!testeco) && (my_it->first != OMNI_PROPERTY_MSC)) || ((testeco) && (my_it->first != OMNI_PROPERTY_TMSC)))) continue;
            md_PricesMap & prices = my_it->second;
            for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it) { // loop through the sell prices for the property
                std::string unitPriceStr;
                int64_t available = 0, total = 0;
                bool includesMe = false;
                md_Set & indexes = (it->second);
                for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it) { // multiple sell offers can exist at the same price, sum them for the UI
                    const CMPMetaDEx& obj = *it;
                    if (obj.displayUnitPrice() == "0.00000000") continue; // hide trades that have a lower than 0.00000001 MSC unit price
                    if(IsMyAddress(obj.getAddr())) includesMe = true;
                    unitPriceStr = obj.displayUnitPrice();
                    if (useBuyList) {
                        if (obj.getDesProperty()==global_metadex_market) {
                            available += obj.getAmountToFill();
                            total += obj.getAmountRemaining();
                        }
                    } else {
                        if ( ((testeco) && (obj.getDesProperty() == 2)) || ((!testeco) && (obj.getDesProperty() == 1)) ) {
                            available += obj.getAmountRemaining();
                            total += obj.getAmountToFill();
                        }
                    }
                }
                if ((available > 0) && (total > 0)) { // if there are any available at this price, add to the sell list
                    string strAvail;
                    if (isPropertyDivisible(global_metadex_market)) { strAvail = FormatDivisibleShortMP(available); } else { strAvail = FormatIndivisibleMP(available); }
                    AddRow(useBuyList, includesMe, StripTrailingZeros(unitPriceStr), strAvail, FormatDivisibleShortMP(total));
                }
            }
        }
    }
}

// This function updates the balance for the currently selected sell address
void MetaDExDialog::UpdateSellAddressBalance()
{
    QString currentSetSellAddress = ui->sellAddressCombo->currentText();
    if (currentSetSellAddress.isEmpty()) {
        ui->yourSellBalanceLabel->setText(QString::fromStdString("Your balance: N/A"));
        ui->sellAddressFeeWarningLabel->setVisible(false);
    } else {
        unsigned int propertyId = global_metadex_market;
        int64_t balanceAvailable = getUserAvailableMPbalance(currentSetSellAddress.toStdString(), propertyId);
        string sellBalStr;
        if (isPropertyDivisible(propertyId)) { sellBalStr = FormatDivisibleMP(balanceAvailable); } else { sellBalStr = FormatIndivisibleMP(balanceAvailable); }
        ui->yourSellBalanceLabel->setText(QString::fromStdString("Your balance: " + sellBalStr + " SPT"));
        // warning label will be lit if insufficient fees for MetaDEx payload (28 bytes)
        if (feeCheck(currentSetSellAddress.toStdString(), 28)) {
            ui->sellAddressFeeWarningLabel->setVisible(false);
        } else {
            ui->sellAddressFeeWarningLabel->setText("WARNING: The address is low on BTC for transaction fees.");
            ui->sellAddressFeeWarningLabel->setVisible(true);
        }
    }
}

// This function updates the balance for the currently selected buy address
void MetaDExDialog::UpdateBuyAddressBalance()
{
    QString currentSetBuyAddress = ui->buyAddressCombo->currentText();
    if (currentSetBuyAddress.isEmpty()) {
        ui->yourBuyBalanceLabel->setText(QString::fromStdString("Your balance: N/A"));
        ui->buyAddressFeeWarningLabel->setVisible(false);
    } else {
        unsigned int propertyId = OMNI_PROPERTY_MSC;
        if (global_metadex_market >= TEST_ECO_PROPERTY_1) propertyId = OMNI_PROPERTY_TMSC;
        int64_t balanceAvailable = getUserAvailableMPbalance(currentSetBuyAddress.toStdString(), propertyId);
        ui->yourBuyBalanceLabel->setText(QString::fromStdString("Your balance: " + FormatDivisibleMP(balanceAvailable) + getTokenLabel(propertyId)));
        // warning label will be lit if insufficient fees for MetaDEx payload (28 bytes)
        if (feeCheck(currentSetBuyAddress.toStdString(), 28)) {
            ui->buyAddressFeeWarningLabel->setVisible(false);
        } else {
            ui->buyAddressFeeWarningLabel->setText("WARNING: The address is low on BTC for transaction fees.");
            ui->buyAddressFeeWarningLabel->setVisible(true);
        }
    }
}

// This function performs a full refresh of all elements - for example when switching markets
void MetaDExDialog::FullRefresh()
{
    // populate market information
    unsigned int propertyId = global_metadex_market;
    string propNameStr = getPropertyName(propertyId);
    bool testeco = false;
    if (propertyId >= TEST_ECO_PROPERTY_1) testeco = true;
    if(testeco) {
        ui->marketLabel->setText(QString::fromStdString("Trade " + propNameStr + " (#" + FormatIndivisibleMP(propertyId) + ") for Test Mastercoin"));
    } else {
        ui->marketLabel->setText(QString::fromStdString("Trade " + propNameStr + " (#" + FormatIndivisibleMP(propertyId) + ") for Mastercoin"));
    }

    // update form labels to reflect market
    std::string primaryToken;
    if (testeco) { primaryToken = "TMSC"; } else { primaryToken = "MSC"; }
    ui->exchangeLabel->setText("Exchange - SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId) + "/" + primaryToken));
    ui->buyTotalLabel->setText("0.00000000 " + QString::fromStdString(primaryToken));
    ui->sellTotalLabel->setText("0.00000000 " + QString::fromStdString(primaryToken));
    ui->sellTM->setText(QString::fromStdString(primaryToken));
    ui->buyTM->setText(QString::fromStdString(primaryToken));
    ui->buyList->setHorizontalHeaderItem(0, new QTableWidgetItem("Total " + QString::fromStdString(primaryToken)));
    ui->buyList->setHorizontalHeaderItem(1, new QTableWidgetItem("Total SP#" + QString::fromStdString(FormatIndivisibleMP(global_metadex_market))));
    ui->buyList->setHorizontalHeaderItem(2, new QTableWidgetItem("Unit Price (" + QString::fromStdString(primaryToken) + ")"));
    ui->sellList->setHorizontalHeaderItem(0, new QTableWidgetItem("Unit Price (" + QString::fromStdString(primaryToken) + ")"));
    ui->sellList->setHorizontalHeaderItem(1, new QTableWidgetItem("Total SP#" + QString::fromStdString(FormatIndivisibleMP(global_metadex_market))));
    ui->sellList->setHorizontalHeaderItem(2, new QTableWidgetItem("Total " + QString::fromStdString(primaryToken)));
    ui->buyMarketLabel->setText("BUY SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId)));
    ui->sellMarketLabel->setText("SELL SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId)));
    ui->sellButton->setText("Sell SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId)));
    ui->buyButton->setText("Buy SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId)));

    // populate buy and sell addresses
    PopulateAddresses();

    // update the buy and sell offers
    UpdateOffers();
}

void MetaDExDialog::buyAddressComboBoxChanged(int idx)
{
    UpdateBuyAddressBalance();
}

void MetaDExDialog::sellAddressComboBoxChanged(int idx)
{
    UpdateSellAddressBalance();
}

void MetaDExDialog::switchButtonClicked()
{
    SwitchMarket();
}

void MetaDExDialog::sellTrade()
{
    sendTrade(true);
}

void MetaDExDialog::buyTrade()
{
    sendTrade(false);
}

void MetaDExDialog::recalcBuyTotal()
{
    recalcTotal(true);
}

void MetaDExDialog::recalcSellTotal()
{
    recalcTotal(false);
}

// This function recalulates a total price display from user fields
void MetaDExDialog::recalcTotal(bool useBuyFields)
{
    unsigned int propertyId = global_metadex_market;
    bool divisible = isPropertyDivisible(propertyId);
    bool testeco = isTestEcosystemProperty(propertyId);
    int64_t price = 0, amount = 0;
    int64_t totalPrice = 0;

    if (useBuyFields) {
        amount = StrToInt64(ui->buyAmountLE->text().toStdString(),divisible);
        price = StrToInt64(ui->buyPriceLE->text().toStdString(),true);
    } else {
        amount = StrToInt64(ui->sellAmountLE->text().toStdString(),divisible);
        price = StrToInt64(ui->sellPriceLE->text().toStdString(),true);
    }
    totalPrice = amount * price;

    // error and overflow detection
    if (0 >= amount || 0 >= price || totalPrice < amount || totalPrice < price || (totalPrice / amount != price)) {
        if (useBuyFields) { ui->buyTotalLabel->setText("N/A"); return; } else { ui->sellTotalLabel->setText("N/A"); return; }
    }

    if (divisible) totalPrice = totalPrice/COIN;
    QString totalLabel = QString::fromStdString(FormatDivisibleMP(totalPrice));
    if (testeco) {
        if (useBuyFields) { ui->buyTotalLabel->setText(totalLabel + " TMSC"); } else { ui->sellTotalLabel->setText(totalLabel + " TMSC"); }
    } else {
        if (useBuyFields) { ui->buyTotalLabel->setText(totalLabel + " MSC"); } else { ui->sellTotalLabel->setText(totalLabel + " MSC"); }
    }
}

void MetaDExDialog::sendTrade(bool sell)
{
    int blockHeight = GetHeight();
    uint32_t propertyId = global_metadex_market;
    bool divisible = isPropertyDivisible(propertyId);
    bool testeco = isTestEcosystemProperty(propertyId);

    // check if main net trading is allowed
    if (!IsTransactionTypeAllowed(blockHeight, propertyId, MSC_TYPE_METADEX_TRADE, MP_TX_PKT_V0)) {
        QMessageBox::critical( this, "Unable to send MetaDEx transaction",
        "Trading on main ecosystem properties is not yet active.\n\nPlease switch to a test ecosystem market to send trade transactions." );
        return;
    }

    // obtain the selected sender address
    string strFromAddress;
    if (!sell) { strFromAddress = ui->buyAddressCombo->currentText().toStdString(); } else { strFromAddress = ui->sellAddressCombo->currentText().toStdString(); }

    // warn if we have to truncate the amount due to a decimal amount for an indivisible property, but allow send to continue
    string strAmount;
    if (!sell) { strAmount = ui->buyAmountLE->text().toStdString(); } else { strAmount = ui->sellAmountLE->text().toStdString(); }
    if (!divisible) {
        size_t pos = strAmount.find(".");
        if (pos!=std::string::npos) {
            string tmpStrAmount = strAmount.substr(0,pos);
            string strMsgText = "The amount entered contains a decimal however the property being transacted is indivisible.\n\nThe amount entered will be truncated as follows:\n";
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

    // use strToInt64 function to get the amounts, using divisibility of the property
    int64_t amountDes;
    int64_t amountSell;
    int64_t price;
    unsigned int propertyIdDes;
    unsigned int propertyIdSell;
    if(sell) {
        amountSell = StrToInt64(ui->sellAmountLE->text().toStdString(),divisible);
        price = StrToInt64(ui->sellPriceLE->text().toStdString(),true);
        if(divisible) { amountDes = (amountSell * price)/COIN; } else { amountDes = amountSell * price; }
        if(testeco) { propertyIdDes = 2; } else { propertyIdDes = 1; }
        propertyIdSell = global_metadex_market;
    } else {
        amountDes = StrToInt64(ui->buyAmountLE->text().toStdString(),divisible);
        price = StrToInt64(ui->buyPriceLE->text().toStdString(),true);
        if(divisible) { amountSell = (amountDes * price)/COIN; } else { amountSell = amountDes * price; }
        if(testeco) { propertyIdSell = 2; } else { propertyIdSell = 1; }
        propertyIdDes = global_metadex_market;
    }
    if ((0>=amountDes) || (0>=amountSell) || (0>=propertyIdDes) || (0>=propertyIdSell)) {
        QMessageBox::critical( this, "Unable to send MetaDEx transaction",
        "The amount entered is not valid.\n\nPlease double-check the transction details thoroughly before retrying your transaction." );
        return;
    }

    // check if sending address has enough funds
    int64_t balanceAvailable = 0;
    balanceAvailable = getUserAvailableMPbalance(strFromAddress, propertyIdSell);
    if (amountSell>balanceAvailable) {
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
    string strMsgText = "You are about to send the following MetaDEx transaction, please check the details thoroughly:\n\n";
    string spNum = static_cast<ostringstream*>( &(ostringstream() << propertyId) )->str();
    string propDetails = "#" + spNum;
    string buyStr;
    string sellStr;
    if (!sell) strMsgText += "Your buy will be inverted into a sell offer.\n\n";
    strMsgText += "Type: Trade Request\nFrom: " + strFromAddress + "\n\n";
    if (!sell) { // clicked buy
        if (divisible) { buyStr = FormatDivisibleMP(amountDes); } else { buyStr = FormatIndivisibleMP(amountDes); }
        buyStr += "   SPT " + propDetails + "";
        sellStr = FormatDivisibleMP(amountSell);
        if (testeco) { sellStr += "   TMSC"; } else { sellStr += "   MSC"; }
        strMsgText += "Buying: " + buyStr + "\nPrice: " + FormatDivisibleMP(price) + "   SP" + propDetails + "/";
        if (testeco) { strMsgText += "TMSC"; } else { strMsgText += "MSC"; }
        strMsgText += "\nTotal: " + sellStr;
    } else { // clicked sell
        buyStr = FormatDivisibleMP(amountDes);
        if (divisible) { sellStr = FormatDivisibleMP(amountSell); } else { sellStr = FormatIndivisibleMP(amountSell); }
        if (testeco) { buyStr += "   TMSC"; } else { buyStr += "   MSC"; }
        sellStr += "   SPT " + propDetails + "";
        strMsgText += "Selling: " + sellStr + "\nPrice: " + FormatDivisibleMP(price) + "   SP" + propDetails + "/";
        if (testeco) { strMsgText += "TMSC"; } else { strMsgText += "MSC"; }
        strMsgText += "\nTotal: " + buyStr;
    }
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
    std::vector<unsigned char> payload = CreatePayload_MetaDExTrade(propertyIdSell, amountSell, propertyIdDes, amountDes);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid = 0;
    std::string rawHex;
    int result = ClassAgnosticWalletTXBuilder(strFromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        string strError = error_str(result);
        QMessageBox::critical( this, "MetaDEx transaction failed",
        "The MetaDEx transaction has failed.\n\nThe error code was: " + QString::number(result) + "\nThe error message was:\n" + QString::fromStdString(strError));
        return;
    } else {
        if (!autoCommit) {
            PopulateSimpleDialog(rawHex, "Raw Hex (auto commit is disabled)", "Raw transaction hex");
        } else {
            PendingAdd(txid, strFromAddress, MSC_TYPE_METADEX_TRADE, propertyIdSell, amountSell);
            PopulateTXSentDialog(txid.GetHex());
        }
    }
}



