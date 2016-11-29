// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "metadexcanceldialog.h"
#include "ui_metadexcanceldialog.h"

#include "omnicore_qtutils.h"

#include "clientmodel.h"
#include "ui_interface.h"
#include "walletmodel.h"

#include "omnicore/createpayload.h"
#include "omnicore/errors.h"
#include "omnicore/mdex.h"
#include "omnicore/omnicore.h"
#include "omnicore/sp.h"
#include "omnicore/pending.h"
#include "omnicore/utilsbitcoin.h"
#include "omnicore/wallettxs.h"

#include <stdint.h>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <QDateTime>
#include <QDialog>
#include <QMessageBox>
#include <QString>
#include <QWidget>

using std::ostringstream;
using std::string;
using namespace mastercore;

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

MetaDExCancelDialog::~MetaDExCancelDialog()
{
    delete ui;
}

/**
 * Sets the client model.
 */
void MetaDExCancelDialog::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if (model != NULL) {
        connect(model, SIGNAL(refreshOmniBalance()), this, SLOT(RefreshUI()));
        connect(model, SIGNAL(reinitOmniState()), this, SLOT(ReinitUI()));
    }
}

/**
 * Sets the wallet model.
 */
void MetaDExCancelDialog::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
}

void MetaDExCancelDialog::ReinitUI()
{
    UpdateAddressSelector();
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
    LOCK(cs_tally);

    QString selectedItem = ui->fromCombo->currentText();
    ui->fromCombo->clear();

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

    // restore initial selection
    int idx = ui->fromCombo->findText(selectedItem);
    if (idx != -1) {
        ui->fromCombo->setCurrentIndex(idx);
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

    ui->cancelCombo->clear();

    bool fMainEcosystem = false;
    bool fTestEcosystem = false;

    LOCK(cs_tally);

    for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it) {
        md_PricesMap & prices = my_it->second;
        for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it) {
            md_Set & indexes = it->second;
            for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it) {
                CMPMetaDEx obj = *it;
                if(senderAddress == obj.getAddr()) {
                    // for "cancel all":
                    if (isMainEcosystemProperty(obj.getProperty())) fMainEcosystem = true;
                    if (isTestEcosystemProperty(obj.getProperty())) fTestEcosystem = true;

                    bool isBuy = false; // sell or buy? (from UI perspective)
                    if ((obj.getProperty() == OMNI_PROPERTY_MSC) || (obj.getProperty() == OMNI_PROPERTY_TMSC)) isBuy = true;
                    string sellToken = getPropertyName(obj.getProperty()).c_str();
                    string desiredToken = getPropertyName(obj.getDesProperty()).c_str();
                    string sellId = strprintf("%d", obj.getProperty());
                    string desiredId = strprintf("%d", obj.getDesProperty());
                    if(sellToken.size()>30) sellToken=sellToken.substr(0,30)+"...";
                    sellToken += " (#" + sellId + ")";
                    if(desiredToken.size()>30) desiredToken=desiredToken.substr(0,30)+"...";
                    desiredToken += " (#" + desiredId + ")";
                    string comboStr = "Cancel all orders ";
                    if (isBuy) { comboStr += "buying " + desiredToken; } else { comboStr += "selling " + sellToken; }
                    string dataStr = sellId + "/" + desiredId;
                    if (ui->radioCancelPrice->isChecked()) { // append price if needed
                        comboStr += " priced at " + StripTrailingZeros(obj.displayUnitPrice());
                        if ((obj.getProperty() == OMNI_PROPERTY_MSC) || (obj.getDesProperty() == OMNI_PROPERTY_MSC)) { comboStr += " OMNI/SPT"; } else { comboStr += " TOMNI/SPT"; }
                        dataStr += ":" + obj.displayUnitPrice();
                    }
                    int index = ui->cancelCombo->findText(QString::fromStdString(comboStr));
                    if ( index == -1 ) { ui->cancelCombo->addItem(QString::fromStdString(comboStr),QString::fromStdString(dataStr)); }
                }
            }
        }
    }

    if (ui->radioCancelEverything->isChecked()) {
        ui->cancelCombo->clear();
        if (fMainEcosystem) ui->cancelCombo->addItem("All active orders in the main ecosystem", 1);
        if (fTestEcosystem) ui->cancelCombo->addItem("All active orders in the test ecosystem", 2);
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

    uint8_t action = 0;
    /*
     * 1 = NEW
     * 2 = CANCEL_AT_PRICE
     * 3 = CANCEL_ALL_FOR_PAIR
     * 4 = CANCEL_EVERYTHING
     */

    if (ui->radioCancelPrice->isChecked()) action = 2;
    if (ui->radioCancelPair->isChecked()) action = 3;
    if (ui->radioCancelEverything->isChecked()) action = 4;
    if (action == 0) {
        // no cancellation method selected
        QMessageBox::critical( this, "Unable to send transaction",
        "Please ensure you have selected a cancellation method and valid cancellation criteria." );
        return;
    }

/** TODO
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

    uint8_t ecosystem = 0;
    uint32_t propertyIdForSale = 0;
    uint32_t propertyIdDesired = 0;

    if (action != 4) {
        try {
            propertyIdForSale = boost::lexical_cast<uint32_t>(propertyIdForSaleStr);
            propertyIdDesired = boost::lexical_cast<uint32_t>(propertyIdDesiredStr);
        } catch(boost::bad_lexical_cast& e) {
            QMessageBox::critical(this, "Unable to send transaction",
                    QString("Failed to parse property identifiers: ").append(e.what()));
            return;
        }
    } else {
        bool ok = false;
        int ecosystemInt = ui->cancelCombo->itemData(ui->cancelCombo->currentIndex()).toInt(&ok);
        if (!ok) {
            QMessageBox::critical(this, "Unable to send transaction", "No ecosystem selected");
            return;
        }
        ecosystem = static_cast<uint8_t>(ecosystemInt);
    }

    int64_t amountForSale = 0, amountDesired = 0;

    if (action == 2) { // do not attempt to reverse calc values from price, pull suitable ForSale/Desired amounts from metadex map
        bool matched = false;

        LOCK(cs_tally);

        for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it) {
            if (my_it->first != propertyIdForSale) { continue; } // move along, this isn't the prop you're looking for
            md_PricesMap & prices = my_it->second;
            for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it) {
                md_Set & indexes = it->second;
                for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it) {
                    CMPMetaDEx obj = *it;
                    if (obj.displayUnitPrice() == priceStr) {
                        amountForSale = obj.getAmountForSale();
                        amountDesired = obj.getAmountDesired();
                        matched = true;
                        break;
                    }
                }
                if (matched) break;
            }
            if (matched) break;
        }
    }

    // confirmation dialog
    string strMsgText = "You are about to send the following MetaDEx trade cancellation transaction, please check the details thoroughly:\n\n";
    strMsgText += "Type: Cancel Trade Request\nFrom: " + fromAddress + "\nAction: ";
    switch (action) {
        case 2: strMsgText += "2 (Cancel by price)\n"; break;
        case 3: strMsgText += "3 (Cancel by pair)\n"; break;
        case 4: strMsgText += "4 (Cancel all)\n"; break;
    }

    std::string messageStr = "Cancel all orders ";
    if (action != 4) {
        string sellToken = getPropertyName(propertyIdForSale).c_str();
        string desiredToken = getPropertyName(propertyIdDesired).c_str();
        string sellId = strprintf("%d", propertyIdForSale);
        string desiredId = strprintf("%d", propertyIdDesired);
        if(sellToken.size()>30) sellToken=sellToken.substr(0,30)+"...";
        sellToken += " (#" + sellId + ")";
        if(desiredToken.size()>30) desiredToken=desiredToken.substr(0,30)+"...";
        desiredToken += " (#" + desiredId + ")";
        if ((propertyIdForSale == OMNI_PROPERTY_MSC) || (propertyIdForSale == OMNI_PROPERTY_TMSC)) { // "buy" order
            messageStr += "buying " + desiredToken;
        } else {
            messageStr += "selling " + sellToken;
        }
        if (action == 2) { // append price if needed - display the first 24 digits
             std::string displayPrice = StripTrailingZeros(priceStr);
             if (displayPrice.size()>24) displayPrice = displayPrice.substr(0,24)+"...";
             messageStr += " priced at " + displayPrice;
             if ((propertyIdForSale == OMNI_PROPERTY_MSC) || (propertyIdDesired == OMNI_PROPERTY_MSC)) { messageStr += " MSC/SPT"; } else { messageStr += " TMSC/SPT"; }
        }
    } else {
        if (isMainEcosystemProperty(ecosystem)) messageStr += "in the main ecosystem";
        if (isTestEcosystemProperty(ecosystem)) messageStr += "in the test ecosystem";
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

    // TODO: restructure and seperate
    // create a payload for the transaction
    std::vector<unsigned char> payload;
    if (action == 2) { // CANCEL_AT_PRICE
        payload = CreatePayload_MetaDExCancelPrice(propertyIdForSale, amountForSale, propertyIdDesired, amountDesired);
    }
    if (action == 3) { // CANCEL_ALL_FOR_PAIR
        payload = CreatePayload_MetaDExCancelPair(propertyIdForSale, propertyIdDesired);
    }
    if (action == 4) { // CANCEL_ALL_FOR_PAIR
        payload = CreatePayload_MetaDExCancelEcosystem(ecosystem);
    }

    // request the wallet build the transaction (and if needed commit it)
    uint256 txid = 0;
    std::string rawHex;
    int result = WalletTxBuilder(fromAddress, "", "", 0, payload, txid, rawHex, autoCommit);

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
            if (action == 2) PendingAdd(txid, fromAddress, MSC_TYPE_METADEX_CANCEL_PRICE, propertyIdForSale, amountForSale, false);
            if (action == 3) PendingAdd(txid, fromAddress, MSC_TYPE_METADEX_CANCEL_PAIR, propertyIdForSale, 0, false);
            if (action == 4) PendingAdd(txid, fromAddress, MSC_TYPE_METADEX_CANCEL_ECOSYSTEM, ecosystem, 0, false);
            PopulateTXSentDialog(txid.GetHex());
        }
    }
**/
}


