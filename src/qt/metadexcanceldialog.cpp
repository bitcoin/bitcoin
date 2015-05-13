// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "metadexcanceldialog.h"
#include "ui_metadexcanceldialog.h"

#include "addresstablemodel.h"
#include "bitcoinunits.h"
#include "coincontroldialog.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "wallet.h"
#include "base58.h"
#include "coincontrol.h"
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
using namespace json_spirit; // since now using Array in mastercore.h this needs to come first

#include "mastercore.h"
using namespace mastercore;

// potentially overzealous using here
using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace leveldb;
// end potentially overzealous using

#include "mastercore_mdex.h"
#include "mastercore_parse_string.h"
#include "mastercore_tx.h"
#include "mastercore_sp.h"
#include "mastercore_errors.h"
#include "omnicore_qtutils.h"
#include "omnicore_createpayload.h"

#include <QDateTime>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextDocument>

MetaDExCancelDialog::MetaDExCancelDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MetaDExCancelDialog),
    clientModel(0),
    walletModel(0)
{
    ui->setupUi(this);

    connect(ui->radioCancelPair, SIGNAL(clicked()),this, SLOT(UpdateCancelCombo()));
    connect(ui->radioCancelPrice, SIGNAL(clicked()),this, SLOT(UpdateCancelCombo()));
    connect(ui->radioCancelEverything, SIGNAL(clicked()),this, SLOT(UpdateCancelCombo()));
    connect(ui->cancelButton, SIGNAL(clicked()),this, SLOT(SendCancelTransaction()));
    connect(ui->fromCombo, SIGNAL(activated(int)), this, SLOT(fromAddressComboBoxChanged(int)));


    // perform initial from address population
    UpdateAddressSelector();
}


/**
 * Sets the client model.
 */
void MetaDExCancelDialog::setClientModel(ClientModel *model)
{
    if (model != NULL) {
        this->clientModel = model;
        connect(model, SIGNAL(refreshOmniState()), this, SLOT(RefreshUI()));
    }
}

/**
 * Sets the wallet model.
 */
void MetaDExCancelDialog::setWalletModel(WalletModel *model)
{
    if (model != NULL) this->walletModel = model;
}

/**
 * Refreshes the cancellation address selector
 *
 * Note: only addresses that have a currently open MetaDEx trade (determined by
 * the metadex map) will be shown in the address selector (cancellations sent from
 * addresses without an open MetaDEx trade are invalid).
 */
void MetaDExCancelDialog::UpdateAddressSelector()
{
    for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it) {
        md_PricesMap & prices = my_it->second;
        for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it) {
            md_Set & indexes = (it->second);
            for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it) {
                CMPMetaDEx obj = *it;
                if(IsMyAddress(obj.getAddr())) { // this address is ours and has an active MetaDEx trade
                    int idx = ui->fromCombo->findText(QString::fromStdString(obj.getAddr())); // avoid adding duplicates
                    if (idx == -1) ui->fromCombo->addItem(QString::fromStdString(obj.getAddr()));
                }
            }
        }
    }
}

/**
 * Refreshes the cancel combo when the address selector is changed
 */
void MetaDExCancelDialog::fromAddressComboBoxChanged(int)
{
    UpdateCancelCombo(); // all that's needed at this stage
}

/**
 * Refreshes the cancel combo with the latest data based on the currently selected
 * radio button.
 */
void MetaDExCancelDialog::UpdateCancelCombo()
{
    string senderAddress = ui->fromCombo->currentText().toStdString();
    QString existingSelection = ui->cancelCombo->currentText();

    if (senderAddress.empty()) {
        return; // no sender address selected, likely no wallet addresses have open MetaDEx trades
    }

    if ((!ui->radioCancelPair->isChecked()) && (!ui->radioCancelPrice->isChecked()) && (!ui->radioCancelEverything->isChecked())) {
        return; // no radio button is selected
    }

    if (ui->radioCancelEverything->isChecked()) {
        ui->cancelCombo->clear();
        ui->cancelCombo->addItem("All currently active sell orders","ALL");
        return; // don't waste time on work we don't need to do for this case
    }

    ui->cancelCombo->clear();
    for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it) {
        md_PricesMap & prices = my_it->second;
        for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it) {
            XDOUBLE price = it->first;
            md_Set & indexes = it->second;
            for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it) {
                CMPMetaDEx obj = *it;
                if(senderAddress == obj.getAddr()) {
                    bool isBuy = false; // sell or buy? (from UI perspective)
                    bool divisible;
                    if ((obj.getProperty() == OMNI_PROPERTY_MSC) || (obj.getProperty() == OMNI_PROPERTY_TMSC)) isBuy = true;
                    string sellToken = getPropertyName(obj.getProperty()).c_str();
                    string desiredToken = getPropertyName(obj.getDesProperty()).c_str();
                    string sellId = static_cast<ostringstream*>( &(ostringstream() << obj.getProperty()) )->str();
                    string desiredId = static_cast<ostringstream*>( &(ostringstream() << obj.getDesProperty()) )->str();
                    if(sellToken.size()>30) sellToken=sellToken.substr(0,30)+"...";
                    sellToken += " (#" + sellId + ")";
                    if(desiredToken.size()>30) desiredToken=desiredToken.substr(0,30)+"...";
                    desiredToken += " (#" + desiredId + ")";
                    string comboStr = "Cancel all orders ";
                    if (isBuy) {
                        comboStr += "buying " + desiredToken;
                        divisible = isPropertyDivisible(obj.getDesProperty());
                    } else {
                        comboStr += "selling " + sellToken;
                        divisible = isPropertyDivisible(obj.getProperty());
                    }
                    string dataStr = sellId + "/" + desiredId;
                    if (ui->radioCancelPrice->isChecked()) { // append price if needed
                        if (isBuy) {
                            if (!divisible) price = price/COIN;
                        } else {
                            if (!divisible) { price = obj.unitPrice()/COIN; } else { price = obj.unitPrice(); }
                        }
                        comboStr += " priced at " + StripTrailingZeros(price.str(16, std::ios_base::fixed)); // limited to 16 digits of precision for display purposes
                        if ((obj.getProperty() == OMNI_PROPERTY_MSC) || (obj.getDesProperty() == OMNI_PROPERTY_MSC)) { comboStr += " MSC/SPT"; } else { comboStr += " TMSC/SPT"; }
                        dataStr += ":" + price.str(DISPLAY_PRECISION_LEN, std::ios_base::fixed);
                    }
                    int index = ui->cancelCombo->findText(QString::fromStdString(comboStr));
                    if ( index == -1 ) { ui->cancelCombo->addItem(QString::fromStdString(comboStr),QString::fromStdString(dataStr)); }
                }
            }
        }
    }
    int idx = ui->cancelCombo->findText(existingSelection, Qt::MatchExactly);
    if (idx != -1) ui->cancelCombo->setCurrentIndex(idx); // if value selected before update and it still exists, reselect it

}

/**
 * Refreshes the UI fields with the most current data - called when the
 * refreshOmniState() signal is received.
 */
void MetaDExCancelDialog::RefreshUI()
{
    UpdateAddressSelector();
    UpdateCancelCombo();
}


/**
 * Takes the data from the fields in the cancellation UI and asks the wallet to construct a
 * MetaDEx cancel transaction.  Then commits & broadcast the created transaction.
 */
void MetaDExCancelDialog::SendCancelTransaction()
{
    std::string fromAddress = ui->fromCombo->currentText().toStdString();
    if (fromAddress.empty()) {
        // no sender address selected
        QMessageBox::critical( this, "Unable to send transaction",
        "Please select the address you would like to send the cancellation transaction from." );
        return;
    }

    int64_t action = 0;
    if (ui->radioCancelPair->isChecked()) action = 2;
    if (ui->radioCancelPrice->isChecked()) action = 3;
    if (ui->radioCancelEverything->isChecked()) action = 4;
    if (action == 0) {
        // no cancellation method selected
        QMessageBox::critical( this, "Unable to send transaction",
        "Please ensure you have selected a cancellation method and valid cancellation criteria." );
        return;
    }

    std::string dataStr = ui->cancelCombo->itemData(ui->cancelCombo->currentIndex()).toString().toStdString();
    size_t slashPos = dataStr.find("/");
    size_t colonPos = dataStr.find(":");
    if ((slashPos==std::string::npos) && (action !=4)) {
        // cancelCombo does not contain valid data - error out and abort
        QMessageBox::critical( this, "Unable to send transaction",
        "Please ensure you have selected valid cancellation criteria." );
        return;
    }

    uint32_t intBlockDate = GetLatestBlockTime();
    QDateTime currentDate = QDateTime::currentDateTime();
    int secs = QDateTime::fromTime_t(intBlockDate).secsTo(currentDate);
    if(secs > 90*60)
    {
        // wallet is still synchronizing, potential lockup if we try to create a transaction now
        QMessageBox::critical( this, "Unable to send transaction",
        "The client is still synchronizing.  Sending transactions can currently be performed only when the client has completed synchronizing." );
        return;
    }

    std::string propertyIdForSaleStr = dataStr.substr(0,slashPos);
    std::string propertyIdDesiredStr = dataStr.substr(slashPos+1,std::string::npos);
    std::string priceStr;
    if (colonPos!=std::string::npos) {
        propertyIdDesiredStr = dataStr.substr(slashPos+1,colonPos-slashPos-1);
        priceStr = dataStr.substr(colonPos+1,std::string::npos);
    }
    unsigned int propertyIdForSale = boost::lexical_cast<uint32_t>(propertyIdForSaleStr);
    unsigned int propertyIdDesired = boost::lexical_cast<uint32_t>(propertyIdDesiredStr);
    int64_t amountForSale = 0, amountDesired = 0;

    if (action == 3) { // do not attempt to reverse calc values from price, pull suitable ForSale/Desired amounts from metadex map
        bool matched = false;
        for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it) {
            if (my_it->first != propertyIdForSale) { continue; } // move along, this isn't the prop you're looking for
            md_PricesMap & prices = my_it->second;
            for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it) {
                XDOUBLE price = it->first;
                XDOUBLE unitPrice;
                md_Set & indexes = it->second;
                for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it) {
                    CMPMetaDEx obj = *it;
                    unitPrice = obj.unitPrice();
                    if ((propertyIdForSale == OMNI_PROPERTY_MSC) || (propertyIdForSale == OMNI_PROPERTY_TMSC)) { // "buy" order
                        if (price.str(DISPLAY_PRECISION_LEN, std::ios_base::fixed) == priceStr) matched=true;
                    } else {
                        if (unitPrice.str(DISPLAY_PRECISION_LEN, std::ios_base::fixed) == priceStr) matched=true;
                    }
                    if (matched) {
                        amountForSale = obj.getAmountForSale();
                        amountDesired = obj.getAmountDesired();
                        break;
                    }
                }
                if (matched) break;
            }
            if (matched) break;
        }
    }

    printf("ForSale \"%s\"=%lu  Desired \"%s\"=%lu  Price \"%s\"  Action %lu  AmountForSale %lu  AmountDesired %lu\n", propertyIdForSaleStr.c_str(), propertyIdForSale, propertyIdDesiredStr.c_str(), propertyIdDesired, priceStr.c_str(), action, amountForSale, amountDesired);

    // confirmation dialog
    string strMsgText = "You are about to send the following MetaDEx trade cancellation transaction, please check the details thoroughly:\n\n";
    strMsgText += "Type: Cancel Trade Request\nFrom: " + fromAddress + "\nAction: ";
    switch (action) {
        case 2: strMsgText += "2 (Cancel by pair)\n"; break;
        case 3: strMsgText += "3 (Cancel by price)\n"; break;
        case 4: strMsgText += "4 (Cancel all)\n"; break;
    }
    string sellToken = getPropertyName(propertyIdForSale).c_str();
    string desiredToken = getPropertyName(propertyIdDesired).c_str();
    string sellId = static_cast<ostringstream*>( &(ostringstream() << propertyIdForSale) )->str();
    string desiredId = static_cast<ostringstream*>( &(ostringstream() << propertyIdDesired) )->str();
    if(sellToken.size()>30) sellToken=sellToken.substr(0,30)+"...";
    sellToken += " (#" + sellId + ")";
    if(desiredToken.size()>30) desiredToken=desiredToken.substr(0,30)+"...";
    desiredToken += " (#" + desiredId + ")";
    string messageStr = "Cancel all orders ";
    if ((propertyIdForSale == OMNI_PROPERTY_MSC) || (propertyIdForSale == OMNI_PROPERTY_TMSC)) { // "buy" order
        messageStr += "buying " + desiredToken;
    } else {
        messageStr += "selling " + sellToken;
    }
    if (action == 3) { // append price if needed - display the first 24 digits
         std::string displayPrice = StripTrailingZeros(priceStr);
         if (displayPrice.size()>24) displayPrice = displayPrice.substr(0,24)+"...";
         messageStr += " priced at " + displayPrice;
         if ((propertyIdForSale == OMNI_PROPERTY_MSC) || (propertyIdDesired == OMNI_PROPERTY_MSC)) { messageStr += " MSC/SPT"; } else { messageStr += " TMSC/SPT"; }
    }
    strMsgText += "Message: " + messageStr;
    strMsgText += "\n\nAre you sure you wish to send this transaction?";
    QString msgText = QString::fromStdString(strMsgText);
    QMessageBox::StandardButton responseClick;
    responseClick = QMessageBox::question(this, "Confirm transaction", msgText, QMessageBox::Yes|QMessageBox::No);
    if (responseClick == QMessageBox::No)
    {
        QMessageBox::critical( this, "MetaDEx cancel aborted",
        "The MetaDEx trade cancellation transaction has been aborted.\n\nPlease double-check the transction details thoroughly before retrying your transaction." );
        return;
    }

    // unlock the wallet
    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled/failed
        QMessageBox::critical( this, "MetaDEx cancel transaction failed",
        "The MetaDEx cancel transaction has been aborted.\n\nThe wallet unlock process must be completed to send a transaction." );
        return;
    }

    // create a payload for the transaction
    std::vector<unsigned char> payload = CreatePayload_MetaDExTrade(propertyIdForSale, amountForSale, propertyIdDesired, amountDesired, action);

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid = 0;
    std::string rawHex;
    int result = ClassAgnosticWalletTXBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        string strError = error_str(result);
        QMessageBox::critical( this, "MetaDEx cancel transaction failed",
        "The MetaDEx cancel transaction has failed.\n\nThe error code was: " + QString::number(result) + "\nThe error message was:\n" + QString::fromStdString(strError));
        return;
    } else {
        if (!autoCommit) {
            PopulateSimpleDialog(rawHex, "Raw Hex (auto commit is disabled)", "Raw transaction hex");
        } else {
            PopulateTXSentDialog(txid.GetHex());
            // no need for a pending object for now, no available balances will be affected until confirmation
        }
    }
}


