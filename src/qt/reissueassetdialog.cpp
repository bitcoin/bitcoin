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

#include <QModelIndex>
#include <QDebug>
#include <validation.h>
#include <QMessageBox>
#include <script/standard.h>
#include <base58.h>
#include <QClipboard>

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
    ui->reissuableBox->setCheckState(Qt::CheckState::Checked);
    ui->ipfsText->hide();
    ui->ipfsHashLabel->hide();
    hideMessage();

    ui->currentDataLabel->setText("<h3><b><u>Current Asset Data</u></b></h3>");
    ui->reissueAssetDataLabel->setText("<h3><b><u>New Updated Changes</u></b></h3>");

    LOCK(cs_main);
    std::vector<std::string> assets;
    GetAllOwnedAssets(assets);

    ui->comboBox->addItem("Select an asset");

    for (auto item : assets)
        ui->comboBox->addItem(QString::fromStdString(item).split("!").first());

    ui->currentAssetData->viewport()->setAutoFillBackground(false);
    ui->currentAssetData->setFrameStyle(QFrame::NoFrame);
    ui->currentAssetData->hide();

    ui->updatedAssetData->viewport()->setAutoFillBackground(false);
    ui->updatedAssetData->setFrameStyle(QFrame::NoFrame);
    ui->updatedAssetData->hide();

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

    const CTxDestination dest = DecodeDestination(ui->addressText->text().toStdString());

    if (!(IsValidDestination(dest) || ui->addressText->text().isEmpty())) {
        showMessage("Invalid Raven Address");
        return;
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

    asset->SetNull();
}

void ReissueAssetDialog::enableDataEntry()
{
    ui->quantitySpinBox->setDisabled(false);
    ui->addressText->setDisabled(false);
    ui->reissuableBox->setDisabled(false);
    ui->ipfsBox->setDisabled(false);
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
        CheckFormState();
    }
    else if (!IsValidDestination(dest)) // Invalid address
    {
        ui->addressText->setStyleSheet("border: 1px solid red");
        showMessage("Warning: Invalid Raven address");
        CheckFormState();
    }
    else // Valid address
    {
        hideMessage();
        CheckFormState();
    }
}

void ReissueAssetDialog::onReissueAssetClicked()
{
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

    std::string ipfsDecoded = "";
    if (hasIPFS)
        ipfsDecoded = DecodeIPFS(ui->ipfsText->text().toStdString());

    CReissueAsset reissueAsset(name.toStdString(), quantity, reissuable ? 1 : 0, ipfsDecoded);

    // Create the transaction and broadcast it
    std::pair<int, std::string> error;
    std::string txid;
    if (!CreateReissueAssetTransaction(model->getWallet(), reissueAsset, address.toStdString(), error, txid))
        showMessage("Invalid: " + QString::fromStdString(error.second));
    else {
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
        msgBox.setText(tr("Reissued ") + name + tr(" with the transaction id:"));
        msgBox.setInformativeText(QString::fromStdString(txid));
        msgBox.exec();

        if (msgBox.clickedButton() == okayButton) {
            close();
        }
    }
}