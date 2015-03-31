// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "metadexdialog.h"
#include "ui_metadexdialog.h"

#include "guiutil.h"
#include "optionsmodel.h"
#include "walletmodel.h"
#include "wallet.h"
#include "base58.h"
#include "ui_interface.h"

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

using namespace json_spirit;
#include "mastercore.h"
using namespace mastercore;

// potentially overzealous using here
using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace leveldb;
// end potentially overzealous using

#include "mastercore_dex.h"
#include "mastercore_tx.h"
#include "mastercore_sp.h"
#include "mastercore_parse_string.h"

#include <boost/math/constants/constants.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>

using boost::multiprecision::int128_t;
using boost::multiprecision::cpp_int;
using boost::multiprecision::cpp_dec_float;
using boost::multiprecision::cpp_dec_float_100;

#include <QDateTime>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextDocument>

MetaDExDialog::MetaDExDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MetaDExDialog),
    model(0)
{
    ui->setupUi(this);
    this->model = model;
    //open
    global_metadex_market = 3;

    //hide pending
//    ui->pendingLabel->setVisible(false);
    //prep lists
    ui->buyList->setColumnCount(3);
    ui->sellList->setColumnCount(3);
//    ui->openOrders->setColumnCount(5);

        //dummy orders
//        const int currentRow = ui->openOrders->rowCount();
//        ui->openOrders->setRowCount(currentRow + 1);
//        ui->openOrders->setItem(currentRow, 0, new QTableWidgetItem("1FakeBitcoinAddressDoNotSend"));
//        ui->openOrders->setItem(currentRow, 1, new QTableWidgetItem("Sell"));
//        ui->openOrders->setItem(currentRow, 2, new QTableWidgetItem("0.00004565"));
//        ui->openOrders->setItem(currentRow, 3, new QTableWidgetItem("345.45643222"));
//        ui->openOrders->setItem(currentRow, 4, new QTableWidgetItem("0.015770081"));

//    ui->openOrders->setHorizontalHeaderItem(0, new QTableWidgetItem("Address"));
//    ui->openOrders->setHorizontalHeaderItem(1, new QTableWidgetItem("Type"));
//    ui->openOrders->setHorizontalHeaderItem(2, new QTableWidgetItem("Unit Price"));
//    ui->openOrders->verticalHeader()->setVisible(false);
    #if QT_VERSION < 0x050000
//       ui->openOrders->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    #else
//       ui->openOrders->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    #endif
//    ui->openOrders->horizontalHeader()->resizeSection(1, 60);
//    ui->openOrders->horizontalHeader()->resizeSection(2, 140);
//    ui->openOrders->horizontalHeader()->resizeSection(3, 140);
//    ui->openOrders->horizontalHeader()->resizeSection(4, 140);
//    ui->openOrders->setShowGrid(false);
//    ui->openOrders->setSelectionBehavior(QAbstractItemView::SelectRows);

    ui->buyList->setHorizontalHeaderItem(0, new QTableWidgetItem("Unit Price"));
    ui->buyList->setHorizontalHeaderItem(1, new QTableWidgetItem("Total SP#3"));
    ui->buyList->setHorizontalHeaderItem(2, new QTableWidgetItem("Total MSC"));
    ui->buyList->verticalHeader()->setVisible(false);
    ui->buyList->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    ui->buyList->setShowGrid(false);
    ui->buyList->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->buyList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->buyList->setSelectionMode(QAbstractItemView::SingleSelection);

    ui->sellList->setHorizontalHeaderItem(0, new QTableWidgetItem("Unit Price"));
    ui->sellList->setHorizontalHeaderItem(1, new QTableWidgetItem("Total SP#3"));
    ui->sellList->setHorizontalHeaderItem(2, new QTableWidgetItem("Total MSC"));
    ui->sellList->verticalHeader()->setVisible(false);
    ui->sellList->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    ui->sellList->setShowGrid(false);
    ui->sellList->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->sellList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->sellList->setSelectionMode(QAbstractItemView::SingleSelection);

    ui->pendingLabel->setVisible(false);

    connect(ui->switchButton, SIGNAL(clicked()), this, SLOT(switchButtonClicked()));
    connect(ui->buyButton, SIGNAL(clicked()), this, SLOT(buyTrade()));
    connect(ui->sellButton, SIGNAL(clicked()), this, SLOT(sellTrade()));
    connect(ui->sellAddressCombo, SIGNAL(activated(int)), this, SLOT(sellAddressComboBoxChanged(int)));
    connect(ui->buyAddressCombo, SIGNAL(activated(int)), this, SLOT(buyAddressComboBoxChanged(int)));
    connect(ui->sellAmountLE, SIGNAL(textEdited(const QString &)), this, SLOT(sellRecalc()));
    connect(ui->sellPriceLE, SIGNAL(textEdited(const QString &)), this, SLOT(sellRecalc()));
    connect(ui->buyAmountLE, SIGNAL(textEdited(const QString &)), this, SLOT(buyRecalc()));
    connect(ui->buyPriceLE, SIGNAL(textEdited(const QString &)), this, SLOT(buyRecalc()));
    connect(ui->sellList, SIGNAL(cellClicked(int,int)), this, SLOT(sellClicked(int,int)));
    connect(ui->buyList, SIGNAL(cellClicked(int,int)), this, SLOT(buyClicked(int,int)));

    FullRefresh();

}

void MetaDExDialog::setModel(WalletModel *model)
{
    this->model = model;
    connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(OrderRefresh()));
}

void MetaDExDialog::OrderRefresh()
{
    UpdateSellOffers();
    UpdateBuyOffers();
    // check for pending transactions, could be more filtered to just trades here
    bool pending = false;
    for(PendingMap::iterator my_it = my_pending.begin(); my_it != my_pending.end(); ++my_it)
    {
        // if we get here there are pending transactions in the wallet, flag warning to MetaDEx
        pending = true;
    }
    if(pending) { ui->pendingLabel->setVisible(true); } else { ui->pendingLabel->setVisible(false); }
}

void MetaDExDialog::SwitchMarket()
{
    uint64_t searchPropertyId = 0;
    // first let's check if we have a searchText, if not do nothing
    string searchText = ui->switchLineEdit->text().toStdString();
    if (searchText.empty()) return;

    // try seeing if we have a numerical search string, if so treat it as a property ID search
    try
    {
        searchPropertyId = boost::lexical_cast<int64_t>(searchText);
    }
    catch(const boost::bad_lexical_cast &e) { return; } // bad cast to number

    if ((searchPropertyId > 0) && (searchPropertyId < 4294967290)) // sanity check
    {
        // check if trying to trade against self
        if ((searchPropertyId == 1) || (searchPropertyId == 2))
        {
            //todo add property cannot be traded against self messgevox
            return;
        }
        // check if property exists
        bool spExists = _my_sps->hasSP(searchPropertyId);
        if (!spExists)
        {
            //todo add property not found messagebox
            return;
        }
        else
        {
            global_metadex_market = searchPropertyId;
            FullRefresh();
        }
    }
    OrderRefresh();
}


void MetaDExDialog::buyClicked(int row, int col)
{
printf("clickedbuyoffer\n");
}

void MetaDExDialog::sellClicked(int row, int col)
{
printf("clickedselloffer\n");
}

void MetaDExDialog::UpdateSellOffers()
{
    ui->sellList->clear();
    int rowcount = 0;
    bool testeco = isTestEcosystemProperty(global_metadex_market);
    for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it)
    {
        // look for the property
        if (my_it->first != global_metadex_market) { continue; }

        // loop prices and list any sells for the right pair
        md_PricesMap & prices = my_it->second;
        for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it)
        {
            //XDOUBLE price = (it->first);
            int64_t available = 0;
            int64_t total = 0;
            bool includesMe = false;
            md_Set & indexes = (it->second);
            // loop through each entry and sum up any sells for the right pair
            for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it)
            {
                CMPMetaDEx obj = *it;
                if ( ((testeco) && (obj.getDesProperty() == 2)) || ((!testeco) && (obj.getDesProperty() == 1)) )
                {
                    available += obj.getAmountForSale();
                    total += obj.getAmountDesired();
                    if(IsMyAddress(obj.getAddr())) includesMe = true;
                }
            }
            // done checking this price, if there are any available/total add to pricebook
            if ((available > 0) && (total > 0))
            {
                // add to pricebook
                double price = (double)total/(double)available;
                QString pstr = QString::fromStdString(FormatPriceMP(price)); //(strprintf("%20.10lf",price));//FormatDivisibleAmount(price));
                QString tstr = QString::fromStdString(FormatDivisibleShortMP(available));
                QString mstr = QString::fromStdString(FormatDivisibleShortMP(total));
                if (!ui->sellList) { printf("metadex dialog error\n"); return; }
                ui->sellList->setRowCount(rowcount+2);
                ui->sellList->setItem(rowcount, 0, new QTableWidgetItem(pstr));
                ui->sellList->setItem(rowcount, 1, new QTableWidgetItem(tstr));
                ui->sellList->setItem(rowcount, 2, new QTableWidgetItem(mstr));
                ui->sellList->item(rowcount, 0)->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
                ui->sellList->item(rowcount, 1)->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
                ui->sellList->item(rowcount, 2)->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);

                if(includesMe)
                {
                    QFont font;
                    font.setBold(true);
                    ui->sellList->item(rowcount, 0)->setFont(font);
                    ui->sellList->item(rowcount, 1)->setFont(font);
                    ui->sellList->item(rowcount, 2)->setFont(font);
                }
                rowcount += 1;
            }
        }
    }
    ui->sellList->setHorizontalHeaderItem(0, new QTableWidgetItem("Unit Price"));
    ui->sellList->setHorizontalHeaderItem(1, new QTableWidgetItem("Total SP#" + QString::fromStdString(FormatIndivisibleMP(global_metadex_market))));
    if (!testeco) { ui->sellList->setHorizontalHeaderItem(2, new QTableWidgetItem("Total MSC")); } else { ui->sellList->setHorizontalHeaderItem(2, new QTableWidgetItem("Total TMSC")); }
}

void MetaDExDialog::UpdateBuyOffers()
{
    ui->buyList->clear();
    int rowcount = 0;
    bool testeco = isTestEcosystemProperty(global_metadex_market);
    for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it)
    {
        // look for the property
        unsigned int mapPropertyId = my_it->first;
        if ( ((testeco) && (mapPropertyId != 2)) || ((!testeco) && (mapPropertyId != 1)) ) continue;

        // loop prices and list any sells for the right pair
        md_PricesMap & prices = my_it->second;
        for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it)
        {
            //XDOUBLE price = (1/it->first);
            double available = 0;
            double total = 0;
            bool includesMe = false;
            md_Set & indexes = (it->second);
            // loop through each entry and sum up any sells for the right pair
            for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it)
            {
                CMPMetaDEx obj = *it;
                if(obj.getDesProperty()==global_metadex_market)
                {
                    available += obj.getAmountDesired();
                    total += obj.getAmountForSale();
                    if(IsMyAddress(obj.getAddr())) includesMe = true;
                }
            }
            // done checking this price, if there are any available/total add to pricebook
            if ((available > 0) && (total > 0))
            {
                // add to pricebook
                double price = (double)total/(double)available;
                QString pstr = QString::fromStdString(FormatPriceMP(price));
                QString tstr = QString::fromStdString(FormatDivisibleShortMP(available));
                QString mstr = QString::fromStdString(FormatDivisibleShortMP(total));
                if (!ui->buyList) { printf("metadex dialog error\n"); return; }
                ui->buyList->setRowCount(rowcount+2);
                ui->buyList->setItem(rowcount, 0, new QTableWidgetItem(pstr));
                ui->buyList->setItem(rowcount, 1, new QTableWidgetItem(tstr));
                ui->buyList->setItem(rowcount, 2, new QTableWidgetItem(mstr));
                ui->buyList->item(rowcount, 0)->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
                ui->buyList->item(rowcount, 1)->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);
                ui->buyList->item(rowcount, 2)->setTextAlignment(Qt::AlignRight + Qt::AlignVCenter);

                if(includesMe)
                {
                    QFont font;
                    font.setBold(true);
                    ui->buyList->item(rowcount, 0)->setFont(font);
                    ui->buyList->item(rowcount, 1)->setFont(font);
                    ui->buyList->item(rowcount, 2)->setFont(font);
                }
                rowcount += 1;
            }
        }
    }
    ui->buyList->setHorizontalHeaderItem(0, new QTableWidgetItem("Unit Price"));
    ui->buyList->setHorizontalHeaderItem(1, new QTableWidgetItem("Total SP#" + QString::fromStdString(FormatIndivisibleMP(global_metadex_market))));
    if (!testeco) { ui->buyList->setHorizontalHeaderItem(2, new QTableWidgetItem("Total MSC")); } else { ui->buyList->setHorizontalHeaderItem(2, new QTableWidgetItem("Total TMSC")); }
}

void MetaDExDialog::UpdateSellAddress()
{
    unsigned int propertyId = global_metadex_market;
    bool divisible = isPropertyDivisible(propertyId);
    QString currentSetSellAddress = ui->sellAddressCombo->currentText();
    int64_t balanceAvailable = getUserAvailableMPbalance(currentSetSellAddress.toStdString(), propertyId);
    string labStr;
    if (divisible)
    {
        labStr = "Your balance: " + FormatDivisibleMP(balanceAvailable) + " SPT";
    }
    else
    {
        labStr = "Your balance: " + FormatIndivisibleMP(balanceAvailable) + " SPT";
    }
    QString qLabStr = QString::fromStdString(labStr);
    ui->yourSellBalanceLabel->setText(qLabStr);
}

void MetaDExDialog::UpdateBuyAddress()
{
    unsigned int propertyId = global_metadex_market;
    bool testeco = false;
    if (propertyId >= TEST_ECO_PROPERTY_1) testeco = true;
    QString currentSetBuyAddress = ui->buyAddressCombo->currentText();
    int64_t balanceAvailable;
    string tokenStr;
    if (testeco)
    {
        balanceAvailable = getUserAvailableMPbalance(currentSetBuyAddress.toStdString(), OMNI_PROPERTY_TMSC);
        tokenStr = " TMSC";
    }
    else
    {
        balanceAvailable = getUserAvailableMPbalance(currentSetBuyAddress.toStdString(), OMNI_PROPERTY_MSC);
        tokenStr = " MSC";

    }
    string labStr = "Your balance: " + FormatDivisibleMP(balanceAvailable) + tokenStr;
    QString qLabStr = QString::fromStdString(labStr);
    ui->yourBuyBalanceLabel->setText(qLabStr);
}

void MetaDExDialog::FullRefresh()
{
    // populate from address selector
    unsigned int propertyId = global_metadex_market;
    string propNameStr = getPropertyName(propertyId);
    bool testeco = false;
    if (propertyId >= TEST_ECO_PROPERTY_1) testeco = true;
    if(testeco)
    {
        ui->marketLabel->setText(QString::fromStdString("Trade " + propNameStr + " (#" + FormatIndivisibleMP(propertyId) + ") for Test Mastercoin"));
    }
    else
    {
        ui->marketLabel->setText(QString::fromStdString("Trade " + propNameStr + " (#" + FormatIndivisibleMP(propertyId) + ") for Mastercoin"));
    }
    LOCK(cs_tally);

    // get currently selected addresses
    QString currentSetBuyAddress = ui->buyAddressCombo->currentText();
    QString currentSetSellAddress = ui->sellAddressCombo->currentText();

    // clear address selectors
    ui->buyAddressCombo->clear();
    ui->sellAddressCombo->clear();

    // update form labels
    if (testeco)
    {
        ui->exchangeLabel->setText("Exchange - SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId)) + "/TMSC");
        ui->buyList->setHorizontalHeaderItem(2, new QTableWidgetItem("TMSC"));
        ui->sellList->setHorizontalHeaderItem(2, new QTableWidgetItem("TMSC"));
        ui->buyTotalLabel->setText("0.00000000 TMSC");
        ui->sellTotalLabel->setText("0.00000000 TMSC");
//        ui->openOrders->setHorizontalHeaderItem(3, new QTableWidgetItem("SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId))));
//        ui->openOrders->setHorizontalHeaderItem(4, new QTableWidgetItem("TMSC"));
        ui->sellTM->setText("TMSC");
        ui->buyTM->setText("TMSC");
    }
    else
    {
        ui->exchangeLabel->setText("Exchange - SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId)) + "/MSC");
        ui->buyList->setHorizontalHeaderItem(2, new QTableWidgetItem("MSC"));
        ui->sellList->setHorizontalHeaderItem(2, new QTableWidgetItem("MSC"));
        ui->buyTotalLabel->setText("0.00000000 MSC");
        ui->sellTotalLabel->setText("0.00000000 MSC");
//        ui->openOrders->setHorizontalHeaderItem(3, new QTableWidgetItem("SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId))));
//        ui->openOrders->setHorizontalHeaderItem(4, new QTableWidgetItem("MSC"));
        ui->sellTM->setText("MSC");
        ui->buyTM->setText("MSC");
    }

    ui->buyMarketLabel->setText("BUY SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId)));
    ui->sellMarketLabel->setText("SELL SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId)));
    ui->buyList->setHorizontalHeaderItem(1, new QTableWidgetItem("SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId))));
    ui->sellList->setHorizontalHeaderItem(1, new QTableWidgetItem("SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId))));
    ui->sellButton->setText("Sell SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId)));
    ui->buyButton->setText("Buy SP#" + QString::fromStdString(FormatIndivisibleMP(propertyId)));

    // sell addresses
    for(map<string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it)
    {
        string address = (my_it->first).c_str();
        unsigned int id;
        bool includeAddress=false;
        (my_it->second).init();
        while (0 != (id = (my_it->second).next()))
        {
            if(id==propertyId) { includeAddress=true; break; }
        }
        if (!includeAddress) continue; //ignore this address, has never transacted in this propertyId
        if (!IsMyAddress(address)) continue; //ignore this address, it's not ours
        ui->sellAddressCombo->addItem((my_it->first).c_str());
    }
    // buy addresses
    for(map<string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it)
    {
        string address = (my_it->first).c_str();
        unsigned int id;
        bool includeAddress=false;
        (my_it->second).init();
        while (0 != (id = (my_it->second).next()))
        {
            if((id==OMNI_PROPERTY_MSC) && (!testeco)) { includeAddress=true; break; }
            if((id==OMNI_PROPERTY_TMSC) && (testeco)) { includeAddress=true; break; }
        }
        if (!includeAddress) continue; //ignore this address, has never transacted in this propertyId
        if (!IsMyAddress(address)) continue; //ignore this address, it's not ours
        ui->buyAddressCombo->addItem((my_it->first).c_str());
    }

    // attempt to set buy and sell addresses back to values before refresh
    int sellIdx = ui->sellAddressCombo->findText(currentSetSellAddress);
    if (sellIdx != -1) { ui->sellAddressCombo->setCurrentIndex(sellIdx); } // -1 means the new prop doesn't have the previously selected address
    int buyIdx = ui->buyAddressCombo->findText(currentSetBuyAddress);
    if (buyIdx != -1) { ui->buyAddressCombo->setCurrentIndex(buyIdx); } // -1 means the new prop doesn't have the previously selected address
    // update the balances
    UpdateSellAddress();
    UpdateBuyAddress();
    UpdateBuyOffers();
    UpdateSellOffers();
    // silly sizing
//    QRect rect = ui->openOrders->geometry();
//    int tableHeight = 2 + ui->openOrders->horizontalHeader()->height();
//    for(int i = 0; i < ui->openOrders->rowCount(); i++){
//        tableHeight += ui->openOrders->rowHeight(i);
//    }
//    rect.setHeight(tableHeight);
//    ui->openOrders->setGeometry(rect);
}

void MetaDExDialog::buyAddressComboBoxChanged(int idx)
{
    UpdateBuyAddress();
}

void MetaDExDialog::sellAddressComboBoxChanged(int idx)
{
    UpdateSellAddress();
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


void MetaDExDialog::buyRecalc()
{
    unsigned int propertyId = global_metadex_market;
    bool divisible = isPropertyDivisible(propertyId);
    bool testeco = false;
    if (propertyId >= TEST_ECO_PROPERTY_1) testeco = true;
    uint64_t buyAmount = StrToInt64(ui->buyAmountLE->text().toStdString(),divisible);
    uint64_t buyPrice = StrToInt64(ui->buyPriceLE->text().toStdString(),true);

    if ((0>=buyAmount) || (0>=buyPrice)) { ui->buyTotalLabel->setText("N/A"); return; } // break out before invalid calc

    //could result in overflow TODO
    uint64_t totalPrice = 0;
    if(divisible) { totalPrice = (buyAmount * buyPrice)/COIN; } else { totalPrice = buyAmount * buyPrice; }

    string totalLabel = FormatDivisibleMP(totalPrice);
    if (testeco)
    {
        ui->buyTotalLabel->setText(QString::fromStdString(totalLabel) + " TMSC");
    }
    else
    {
        ui->buyTotalLabel->setText(QString::fromStdString(totalLabel) + " MSC");
    }
}

void MetaDExDialog::sellRecalc()
{
    unsigned int propertyId = global_metadex_market;
    bool divisible = isPropertyDivisible(propertyId);
    bool testeco = false;
    if (propertyId >= TEST_ECO_PROPERTY_1) testeco = true;
    uint64_t sellAmount = StrToInt64(ui->sellAmountLE->text().toStdString(),divisible);
    uint64_t sellPrice = StrToInt64(ui->sellPriceLE->text().toStdString(),true);
    if ((0>=sellAmount) || (0>=sellPrice)) { ui->sellTotalLabel->setText("N/A"); return; } // break out before invalid calc

    //could result in overflow TODO
    uint64_t totalPrice;
    if(divisible) { totalPrice = (sellAmount * sellPrice)/COIN; } else { totalPrice = sellAmount * sellPrice; }

    string totalLabel = FormatDivisibleMP(totalPrice);
    if (testeco)
    {
        ui->sellTotalLabel->setText(QString::fromStdString(totalLabel) + " TMSC");
    }
    else
    {
        ui->sellTotalLabel->setText(QString::fromStdString(totalLabel) + " MSC");
    }
}

void MetaDExDialog::sendTrade(bool sell)
{
    unsigned int propertyId = global_metadex_market;
    bool divisible = isPropertyDivisible(propertyId);
    bool testeco = false;
    if (propertyId >= TEST_ECO_PROPERTY_1) testeco = true;

    // obtain the selected sender address
    string strFromAddress;
    if (!sell) { strFromAddress = ui->buyAddressCombo->currentText().toStdString(); } else { strFromAddress = ui->sellAddressCombo->currentText().toStdString(); }
    // push recipient address into a CBitcoinAddress type and check validity
    CBitcoinAddress fromAddress;
    if (false == strFromAddress.empty()) { fromAddress.SetString(strFromAddress); }
    if (!fromAddress.IsValid())
    {
        QMessageBox::critical( this, "Unable to send MetaDEx transaction",
        "The sender address selected is not valid.\n\nPlease double-check the transction details thoroughly before retrying your transaction." );
        return;
    }

    // warn if we have to truncate the amount due to a decimal amount for an indivisible property, but allow send to continue

// need to handle sell too
    string strAmount = ui->buyAmountLE->text().toStdString();
    if (!divisible)
    {
        size_t pos = strAmount.find(".");
        if (pos!=std::string::npos)
        {
            string tmpStrAmount = strAmount.substr(0,pos);
            string strMsgText = "The amount entered contains a decimal however the property being transacted is indivisible.\n\nThe amount entered will be truncated as follows:\n";
            strMsgText += "Original amount entered: " + strAmount + "\nAmount that will be used: " + tmpStrAmount + "\n\n";
            strMsgText += "Do you still wish to proceed with the transaction?";
            QString msgText = QString::fromStdString(strMsgText);
            QMessageBox::StandardButton responseClick;
            responseClick = QMessageBox::question(this, "Amount truncation warning", msgText, QMessageBox::Yes|QMessageBox::No);
            if (responseClick == QMessageBox::No)
            {
                QMessageBox::critical( this, "MetaDEx transaction cancelled",
                "The MetaDEx transaction has been cancelled.\n\nPlease double-check the transction details thoroughly before retrying your transaction." );
                return;
            }
            strAmount = tmpStrAmount;
            ui->buyAmountLE->setText(QString::fromStdString(strAmount));
        }
    }

    // use strToInt64 function to get the amounts, using divisibility of the property
    // make fields for trade
    uint64_t amountDes;
    uint64_t amountSell;
    uint64_t price;
    unsigned int propertyIdDes;
    unsigned int propertyIdSell;

    if(sell)
    {
        amountSell = StrToInt64(ui->sellAmountLE->text().toStdString(),divisible);
        price = StrToInt64(ui->sellPriceLE->text().toStdString(),true);
        if(divisible) { amountDes = (amountSell * price)/COIN; } else { amountDes = amountSell * price; }
        if(testeco) { propertyIdDes = 2; } else { propertyIdDes = 1; }
        propertyIdSell = global_metadex_market;
    }
    else
    {
        amountDes = StrToInt64(ui->buyAmountLE->text().toStdString(),divisible);
        price = StrToInt64(ui->buyPriceLE->text().toStdString(),true);
        if(divisible) { amountSell = (amountDes * price)/COIN; } else { amountSell = amountDes * price; }
        if(testeco) { propertyIdSell = 2; } else { propertyIdSell = 1; }
        propertyIdDes = global_metadex_market;
    }

    if ((0>=amountDes) || (0>=amountSell) || (0>=propertyIdDes) || (0>=propertyIdSell))
    {
        QMessageBox::critical( this, "Unable to send MetaDEx transaction",
        "The amount entered is not valid.\n\nPlease double-check the transction details thoroughly before retrying your transaction." );
        return;
    }

    // check if sending address has enough funds
    int64_t balanceAvailable = 0;
    balanceAvailable = getUserAvailableMPbalance(fromAddress.ToString(), propertyIdSell);
    if (amountSell>balanceAvailable)
    {
        QMessageBox::critical( this, "Unable to send MetaDEx transaction",
        "The selected sending address does not have a sufficient balance to cover the amount entered.\n\nPlease double-check the transction details thoroughly before retrying your transaction." );
        return;
    }

    // check if wallet is still syncing, as this will currently cause a lockup if we try to send - compare our chain to peers to see if we're up to date
    // Bitcoin Core devs have removed GetNumBlocksOfPeers, switching to a time based best guess scenario
    uint32_t intBlockDate = GetLatestBlockTime();  // uint32, not using time_t for portability
    QDateTime currentDate = QDateTime::currentDateTime();
    int secs = QDateTime::fromTime_t(intBlockDate).secsTo(currentDate);
    if(secs > 90*60)
    {
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
    strMsgText += "Type: Trade Request\nFrom: " + fromAddress.ToString() + "\n\n";
    if (!sell) // clicked buy
    {
        if (divisible) { buyStr = FormatDivisibleMP(amountDes); } else { buyStr = FormatIndivisibleMP(amountDes); }
        buyStr += "   SPT " + propDetails + "";
        sellStr = FormatDivisibleMP(amountSell);
        if (testeco) { sellStr += "   TMSC"; } else { sellStr += "   MSC"; }
        strMsgText += "Buying: " + buyStr + "\nPrice: " + FormatDivisibleMP(price) + "   SP" + propDetails + "/";
        if (testeco) { strMsgText += "TMSC"; } else { strMsgText += "MSC"; }
        strMsgText += "\nTotal: " + sellStr;
    }
    else // clicked sell
    {
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
    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled/failed
        QMessageBox::critical( this, "MetaDEx transaction failed",
        "The transaction has been cancelled.\n\nThe wallet unlock process must be completed to send a transaction." );
        return;
    }

    // send the transaction - UI will not send any extra reference amounts at this stage
    int code = 0;
//    uint256 sendTXID = send_INTERNAL_1packet(fromAddress.ToString(), refAddress.ToString(), fromAddress.ToString(), propertyId, sendAmount, 0, 0, MSC_TYPE_SIMPLE_SEND, 0, &code);
    uint256 tradeTXID = send_INTERNAL_1packet(fromAddress.ToString(), "", fromAddress.ToString(), propertyIdSell, amountSell, propertyIdDes, amountDes, MSC_TYPE_METADEX, 1, &code);

    if (0 != code)
    {
        string strCode = boost::lexical_cast<string>(code);
        string strError;
        switch(code)
        {
            case -212:
                strError = "Error choosing inputs for the send transaction";
                break;
            case -233:
                strError = "Error with redemption address";
                break;
            case -220:
                strError = "Error with redemption address key ID";
                break;
            case -221:
                strError = "Error obtaining public key for redemption address";
                break;
            case -222:
                strError = "Error public key for redemption address is not valid";
                break;
            case -223:
                strError = "Error validating redemption address";
                break;
            case -205:
                strError = "Error with wallet object";
                break;
            case -206:
                strError = "Error with selected inputs for the send transaction";
                break;
            case -211:
                strError = "Error creating transaction (wallet may be locked or fees may not be sufficient)";
                break;
            case -213:
                strError = "Error committing transaction";
                break;
        }
        if (strError.empty()) strError = "Error code does not have associated error text.";
        QMessageBox::critical( this, "Send transaction failed",
        "The send transaction has failed.\n\nThe error code was: " + QString::fromStdString(strCode) + "\nThe error message was:\n" + QString::fromStdString(strError));
        return;
    }
    else
    {
        // call an update of the balances
//        set_wallet_totals();
//        updateBalances();

        // display the result
        string strSentText = "Your Master Protocol transaction has been sent.\n\nThe transaction ID is:\n\n";
        strSentText += tradeTXID.GetHex() + "\n\n";
        QString sentText = QString::fromStdString(strSentText);
        QMessageBox sentDialog;
        sentDialog.setIcon(QMessageBox::Information);
        sentDialog.setWindowTitle("Transaction broadcast successfully");
        sentDialog.setText(sentText);
        sentDialog.setStandardButtons(QMessageBox::Yes|QMessageBox::Ok);
        sentDialog.setDefaultButton(QMessageBox::Ok);
        sentDialog.setButtonText( QMessageBox::Yes, "Copy TXID to clipboard" );
        if(sentDialog.exec() == QMessageBox::Yes)
        {
            // copy TXID to clipboard
            GUIUtil::setClipboard(QString::fromStdString(tradeTXID.GetHex()));
        }
        // clear the form
//        clearFields();
    }

}


