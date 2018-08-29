// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "reissueassetdialog.h"
#include "ui_reissueassetdialog.h"
#include "platformstyle.h"
#include "walletmodel.h"
#include "addresstablemodel.h"
#include "core_io.h"
#include "univalue.h"
#include "assets/assettypes.h"
#include "ravenunits.h"
#include "optionsmodel.h"
#include "sendcoinsdialog.h"

#include <QModelIndex>
#include <QDebug>
#include <validation.h>
#include <QMessageBox>
#include <script/standard.h>
#include <base58.h>
#include <QClipboard>
#include <wallet/wallet.h>
#include <policy/policy.h>

ReissueAssetDialog::ReissueAssetDialog(const PlatformStyle *_platformStyle, QWidget *parent, WalletModel* model) :
        QDialog(parent),
        ui(new Ui::ReissueAssetDialog),
        platformStyle(_platformStyle)
{
    this->model = model;
    ui->setupUi(this);
    setWindowTitle("Reissue Assets");
    connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(onCloseClicked()));
    connect(ui->comboBox, SIGNAL(activated(int)), this, SLOT(onAssetSelected(int)));
    connect(ui->quantitySpinBox, SIGNAL(valueChanged(double)), this, SLOT(onQuantityChanged(double)));
    connect(ui->ipfsBox, SIGNAL(clicked()), this, SLOT(onIPFSStateChanged()));
    connect(ui->ipfsText, SIGNAL(textChanged(QString)), this, SLOT(onIPFSHashChanged(QString)));
    connect(ui->addressText, SIGNAL(textChanged(QString)), this, SLOT(onAddressNameChanged(QString)));
    connect(ui->reissueAssetButton, SIGNAL(clicked()), this, SLOT(onReissueAssetClicked()));
    connect(ui->changeAddressBox, SIGNAL(clicked()), this, SLOT(onChangeAddressChanged()));
    connect(ui->changeAddressText, SIGNAL(textChanged(QString)), this, SLOT(onChangeAddressTextChanged(QString)));
    connect(ui->reissuableBox, SIGNAL(clicked()), this, SLOT(onReissueBoxChanged()));
    this->asset = new CNewAsset();
    asset->SetNull();
    // Setup the default values
    setUpValues();
}

ReissueAssetDialog::~ReissueAssetDialog()
{
    delete ui;
    delete asset;
}

/** Helper Methods */
void ReissueAssetDialog::setUpValues()
{
    if (!model)
        return;

    ui->reissuableBox->setCheckState(Qt::CheckState::Checked);
    ui->ipfsText->hide();
    ui->ipfsHashLabel->hide();
    hideMessage();

    ui->changeAddressText->hide();

    ui->currentDataLabel->setText("<h3><b><u>Current Asset Data</u></b></h3>");
    ui->reissueAssetDataLabel->setText("<h3><b><u>New Updated Changes</u></b></h3>");

    LOCK(cs_main);
    std::vector<std::string> assets;
    GetAllAdministrativeAssets(model->getWallet(), assets);

    ui->comboBox->addItem("Select an asset");

    // Load the assets that are reissuable
    for (auto item : assets) {
        std::string name = QString::fromStdString(item).split("!").first().toStdString();
        CNewAsset asset;
        if (passets->GetAssetIfExists(name, asset)) {
            if (asset.nReissuable)
                ui->comboBox->addItem(QString::fromStdString(asset.strName));
        }
    }

    // Hide the current asset data
    ui->currentAssetData->viewport()->setAutoFillBackground(false);
    ui->currentAssetData->setFrameStyle(QFrame::NoFrame);
    ui->currentAssetData->hide();

    // Hide the updated asset data
    ui->updatedAssetData->viewport()->setAutoFillBackground(false);
    ui->updatedAssetData->setFrameStyle(QFrame::NoFrame);
    ui->updatedAssetData->hide();

    // Hide the reissue Warning Label
    ui->reissueWarningLabel->hide();

    disableAll();
}

void ReissueAssetDialog::toggleIPFSText()
{
    if (ui->ipfsBox->isChecked()) {
        ui->ipfsHashLabel->show();
        ui->ipfsText->show();
    } else {
        ui->ipfsHashLabel->hide();
        ui->ipfsText->hide();
        ui->ipfsText->clear();
    }

    buildUpdatedData();
    CheckFormState();
}

void ReissueAssetDialog::showMessage(QString string)
{
    ui->messageLabel->setStyleSheet("color: red");
    ui->messageLabel->setText(string);
    ui->messageLabel->show();
}

void ReissueAssetDialog::showValidMessage(QString string)
{
    ui->messageLabel->setStyleSheet("color: green");
    ui->messageLabel->setText(string);
    ui->messageLabel->show();
}

void ReissueAssetDialog::hideMessage()
{
    ui->messageLabel->hide();
}

void ReissueAssetDialog::disableReissueButton()
{
    ui->reissueAssetButton->setDisabled(true);
}

void ReissueAssetDialog::enableReissueButton()
{
    ui->reissueAssetButton->setDisabled(false);
}

void ReissueAssetDialog::CheckFormState()
{
    disableReissueButton(); // Disable the button by default

    // If asset is null
    if (asset->strName == "") {
        showMessage("Asset data couldn't be found");
        return;
    }

    // If the quantity is to large
    if (asset->nAmount + ui->quantitySpinBox->value() * COIN > MAX_MONEY) {
        showMessage("Quantity is to large. Max is 21,000,000,000");
        return;
    }

    // Check the destination address
    const CTxDestination dest = DecodeDestination(ui->addressText->text().toStdString());
    if (!ui->addressText->text().isEmpty()) {
        if (!IsValidDestination(dest)) {
            showMessage("Invalid Raven Destination Address");
            return;
        }
    }

    // Check the change address
    if (ui->changeAddressBox->isChecked())
    {
        if (!ui->changeAddressText->text().isEmpty()) {
            const CTxDestination changeDest = DecodeDestination(ui->changeAddressText->text().toStdString());
            if (!IsValidDestination(changeDest)) {
                showMessage("Invalid Raven Change Address");
                return;
            }
        }
    }

    if (ui->ipfsBox->isChecked() && ui->ipfsText->text().size() != 46) {
        showMessage("Invalid IPFS Hash");
        return;
    }
    enableReissueButton();
    hideMessage();
}

void ReissueAssetDialog::disableAll()
{
    ui->quantitySpinBox->setDisabled(true);
    ui->addressText->setDisabled(true);
    ui->reissuableBox->setDisabled(true);
    ui->ipfsBox->setDisabled(true);
    ui->reissueAssetButton->setDisabled(true);
    ui->changeAddressBox->setDisabled(true);

    asset->SetNull();
}

void ReissueAssetDialog::enableDataEntry()
{
    ui->quantitySpinBox->setDisabled(false);
    ui->addressText->setDisabled(false);
    ui->reissuableBox->setDisabled(false);
    ui->ipfsBox->setDisabled(false);
    ui->changeAddressBox->setDisabled(false);
}

void ReissueAssetDialog::buildUpdatedData()
{
    // Get the display value for the asset quantity
    auto value = ValueFromAmount(asset->nAmount, asset->units);

    double newValue = value.get_real() + ui->quantitySpinBox->value();

    std::stringstream ss;
    ss.precision(asset->units);
    ss << std::fixed << newValue;

    QString ipfs = asset->nHasIPFS ? "true" : "false";
    QString reissuable = ui->reissuableBox->isChecked() ? "true" : "false";
    QString data = QString("Name: %1\n").arg(QString::fromStdString(asset->strName));
    data += QString("Quantity: %1\n").arg(QString::fromStdString(ss.str()));
    data += QString("Reissuable: %1\n").arg(reissuable);
    if (asset->nHasIPFS && !ui->ipfsBox->isChecked())
        data += QString("IPFS Hash: %1\n").arg(QString::fromStdString(EncodeIPFS(asset->strIPFSHash)));
    else if (ui->ipfsBox->isChecked()) {
        data += QString("IPFS Hash: %1\n").arg(ui->ipfsText->text());
    }

    ui->updatedAssetData->setText(data);
    ui->updatedAssetData->show();
}

/** SLOTS */
void ReissueAssetDialog::onCloseClicked()
{
    this->close();
}

void ReissueAssetDialog::onAssetSelected(int index)
{
    // Only display asset information when as asset is clicked. The first index is a PlaceHolder
    if (index > 0) {
        enableDataEntry();
        ui->currentAssetData->show();
        QString name = ui->comboBox->currentText();

        LOCK(cs_main);
        // Get the asset data
        if (!passets->GetAssetIfExists(name.toStdString(), *asset)) {
            CheckFormState();
            disableAll();
            asset->SetNull();
            ui->currentAssetData->hide();
            ui->currentAssetData->clear();
            return;
        }

        // Get the display value for the asset quantity
        auto value = ValueFromAmount(asset->nAmount, asset->units);
        std::stringstream ss;
        ss.precision(asset->units);
        ss << std::fixed << value.get_real();

        ui->quantitySpinBox->setMaximum(21000000000 - value.get_real());

        // Create the QString to display to the user
        QString ipfs = asset->nHasIPFS ? "true" : "false";
        QString data = QString("Name: %1\n").arg(QString::fromStdString(asset->strName));
        data += QString("Quantity: %1\n").arg(QString::fromStdString(ss.str()));
        if (asset->nHasIPFS)
            data += QString("IPFS Hash: %1\n").arg(QString::fromStdString(EncodeIPFS(asset->strIPFSHash)));

        // Show the display text to the user
        ui->currentAssetData->setText(data);
        buildUpdatedData();
        CheckFormState();
    } else {
        disableAll();
        asset->SetNull();
        ui->currentAssetData->hide();
        ui->currentAssetData->clear();
        ui->updatedAssetData->hide();
        ui->updatedAssetData->clear();
    }
}

void ReissueAssetDialog::onQuantityChanged(double qty)
{
    buildUpdatedData();
    CheckFormState();
}

void ReissueAssetDialog::onIPFSStateChanged()
{
    toggleIPFSText();
}

void ReissueAssetDialog::onIPFSHashChanged(QString hash)
{
    if (hash.size() == 0) {
        hideMessage();
        CheckFormState();
    }
    else if (hash.size() != 46) {
        ui->ipfsText->setStyleSheet("border: 1px solid red");
        showMessage("IPFS hash must be the correct length");
    } else {
        hideMessage();
        ui->ipfsText->setStyleSheet("");
        CheckFormState();
    }

    buildUpdatedData();
}

void ReissueAssetDialog::onAddressNameChanged(QString address)
{
    const CTxDestination dest = DecodeDestination(address.toStdString());

    if (address.isEmpty()) // Nothing entered
    {
        hideMessage();
        ui->addressText->setStyleSheet("");
        CheckFormState();
    }
    else if (!IsValidDestination(dest)) // Invalid address
    {
        ui->addressText->setStyleSheet("border: 1px solid red");
        CheckFormState();
    }
    else // Valid address
    {
        hideMessage();
        ui->addressText->setStyleSheet("");
        CheckFormState();
    }
}

void ReissueAssetDialog::onReissueAssetClicked()
{
    if (!model)
        return;

    QString address;
    if (ui->addressText->text().isEmpty()) {
        address = model->getAddressTableModel()->addRow(AddressTableModel::Receive, "", "");
    } else {
        address = ui->addressText->text();
    }

    QString name = ui->comboBox->currentText();
    CAmount quantity = ui->quantitySpinBox->value() * COIN;
    bool reissuable = ui->reissuableBox->isChecked();
    bool hasIPFS = ui->ipfsBox->isChecked();

    QString changeAddress = "";
    if (ui->changeAddressBox->isChecked())
        changeAddress = ui->changeAddressText->text();

    std::string ipfsDecoded = "";
    if (hasIPFS)
        ipfsDecoded = DecodeIPFS(ui->ipfsText->text().toStdString());

    CReissueAsset reissueAsset(name.toStdString(), quantity, reissuable ? 1 : 0, ipfsDecoded);

    CWalletTx tx;
    CReserveKey reservekey(model->getWallet());
    std::pair<int, std::string> error;
    CAmount nFeeRequired;

    // Create the transaction
    if (!CreateReissueAssetTransaction(model->getWallet(), reissueAsset, address.toStdString(), changeAddress.toStdString(), error, tx, reservekey, nFeeRequired)) {
        showMessage("Invalid: " + QString::fromStdString(error.second));
        return;
    }

    // Format confirmation message
    QStringList formatted;

    // generate bold amount string
    QString amount = "<b>" + QString::fromStdString(ValueFromAmountString(GetReissueAssetBurnAmount(), 8)) + " RVN";
    amount.append("</b>");
    // generate monospace address string
    QString addressburn = "<span style='font-family: monospace;'>" + QString::fromStdString(Params().ReissueAssetBurnAddress());
    addressburn.append("</span>");

    QString recipientElement1;
    recipientElement1 = tr("%1 to %2").arg(amount, addressburn);
    formatted.append(recipientElement1);

    // generate the bold asset amount
    QString assetAmount = "<b>" + QString::fromStdString(ValueFromAmountString(reissueAsset.nAmount, 8)) + " " + QString::fromStdString(reissueAsset.strName);
    assetAmount.append("</b>");

    // generate the monospace address string
    QString assetAddress = "<span style='font-family: monospace;'>" + address;
    assetAddress.append("</span>");

    QString recipientElement2;
    recipientElement2 = tr("%1 to %2").arg(assetAmount, assetAddress);
    formatted.append(recipientElement2);

    QString questionString = tr("Are you sure you want to send?");
    questionString.append("<br /><br />%1");

    if(nFeeRequired > 0)
    {
        // append fee string if a fee is required
        questionString.append("<hr /><span style='color:#aa0000;'>");
        questionString.append(RavenUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), nFeeRequired));
        questionString.append("</span> ");
        questionString.append(tr("added as transaction fee"));

        // append transaction size
        questionString.append(" (" + QString::number((double)GetVirtualTransactionSize(tx) / 1000) + " kB)");
    }

    // add total amount in all subdivision units
    questionString.append("<hr />");
    CAmount totalAmount = GetReissueAssetBurnAmount() + nFeeRequired;
    QStringList alternativeUnits;
    for (RavenUnits::Unit u : RavenUnits::availableUnits())
    {
        if(u != model->getOptionsModel()->getDisplayUnit())
            alternativeUnits.append(RavenUnits::formatHtmlWithUnit(u, totalAmount));
    }
    questionString.append(tr("Total Amount %1")
                                  .arg(RavenUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), totalAmount)));
    questionString.append(QString("<span style='font-size:10pt;font-weight:normal;'><br />(=%2)</span>")
                                  .arg(alternativeUnits.join(" " + tr("or") + "<br />")));

    SendConfirmationDialog confirmationDialog(tr("Confirm reissue assets"),
                                              questionString.arg(formatted.join("<br />")), SEND_CONFIRM_DELAY, this);
    confirmationDialog.exec();
    QMessageBox::StandardButton retval = (QMessageBox::StandardButton)confirmationDialog.result();

    if(retval != QMessageBox::Yes)
    {
        return;
    }

    // Create the transaction and broadcast it
    std::string txid;
    if (!SendAssetTransaction(model->getWallet(), tx, reservekey, error, txid)) {
        showMessage("Invalid: " + QString::fromStdString(error.second));
    } else {
        QMessageBox msgBox;
        QPushButton *copyButton = msgBox.addButton(tr("Copy"), QMessageBox::ActionRole);
        copyButton->disconnect();
        connect(copyButton, &QPushButton::clicked, this, [=](){
            QClipboard *p_Clipboard = QApplication::clipboard();
            p_Clipboard->setText(QString::fromStdString(txid), QClipboard::Mode::Clipboard);

            QMessageBox copiedBox;
            copiedBox.setText(tr("Transaction ID Copied"));
            copiedBox.exec();
        });

        QPushButton *okayButton = msgBox.addButton(QMessageBox::Ok);
        msgBox.setText(tr("Asset transaction sent to network:"));
        msgBox.setInformativeText(QString::fromStdString(txid));
        msgBox.exec();

        if (msgBox.clickedButton() == okayButton) {
            close();
        }
    }
}

void ReissueAssetDialog::onChangeAddressChanged()
{
    if (ui->changeAddressBox->isChecked()) {
        ui->changeAddressText->show();
    } else {
        ui->changeAddressText->hide();
    }
}

void ReissueAssetDialog::onChangeAddressTextChanged(QString address)
{
    const CTxDestination dest = DecodeDestination(address.toStdString());

    if (address.isEmpty()) // Nothing entered
    {
        hideMessage();
        ui->changeAddressText->setStyleSheet("");
        CheckFormState();
    }
    else if (!IsValidDestination(dest)) // Invalid address
    {
        ui->changeAddressText->setStyleSheet("border: 1px solid red");
        CheckFormState();
    }
    else // Valid address
    {
        hideMessage();
        ui->changeAddressText->setStyleSheet("");
        CheckFormState();
    }
}

void ReissueAssetDialog::onReissueBoxChanged()
{
    if (!ui->reissuableBox->isChecked()) {
        ui->reissueWarningLabel->setText("Warning: Once this asset is reissued with the reissuable flag set to false. It won't be able to be reissued in the future");
        ui->reissueWarningLabel->setStyleSheet("color: red");
        ui->reissueWarningLabel->show();
    } else {
        ui->reissueWarningLabel->hide();
    }

    buildUpdatedData();
}