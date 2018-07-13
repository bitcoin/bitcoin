// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "createassetdialog.h"
#include "ui_createassetdialog.h"
#include "platformstyle.h"
#include "walletmodel.h"
#include "addresstablemodel.h"

#include <QModelIndex>
#include <QDebug>
#include <validation.h>
#include <QMessageBox>
#include <script/standard.h>
#include <base58.h>
#include <QClipboard>

CreateAssetDialog::CreateAssetDialog(const PlatformStyle *_platformStyle, QWidget *parent, WalletModel* model) :
        QDialog(parent),
        ui(new Ui::CreateAssetDialog),
        platformStyle(_platformStyle)
{
    this->model = model;
    ui->setupUi(this);
    setWindowTitle("Create Assets");
    connect(ui->ipfsBox, SIGNAL(clicked()), this, SLOT(ipfsStateChanged()));
    connect(ui->availabilityButton, SIGNAL(clicked()), this, SLOT(checkAvailabilityClicked()));
    connect(ui->nameText, SIGNAL(textChanged(QString)), this, SLOT(onNameChanged(QString)));
    connect(ui->addressText, SIGNAL(textChanged(QString)), this, SLOT(onAddressNameChanged(QString)));
    connect(ui->ipfsText, SIGNAL(textChanged(QString)), this, SLOT(onIPFSHashChanged(QString)));
    connect(ui->closeButton, SIGNAL(clicked()), this, SLOT(onCloseClicked()));
    connect(ui->createAssetButton, SIGNAL(clicked()), this, SLOT(onCreateAssetClicked()));
    connect(ui->unitBox, SIGNAL(valueChanged(int)), this, SLOT(onUnitChanged(int)));

    // Setup the default values
    setUpValues();
}

CreateAssetDialog::~CreateAssetDialog()
{
    delete ui;
}

/** Helper Methods */
void CreateAssetDialog::setUpValues()
{
    ui->unitBox->setValue(8);
    ui->reissuableBox->setCheckState(Qt::CheckState::Checked);
    ui->ipfsText->hide();
    ui->ipfsHashLabel->hide();
    ui->ipfsLine->hide();
    ui->availabilityButton->setDisabled(true);
    hideMessage();
    CheckFormState();

    ui->unitExampleLabel->setStyleSheet("font-weight: bold");
}

void CreateAssetDialog::toggleIPFSText()
{
    if (ui->ipfsBox->isChecked()) {
        ui->ipfsHashLabel->show();
        ui->ipfsText->show();
        ui->ipfsLine->show();
    } else {
        ui->ipfsHashLabel->hide();
        ui->ipfsText->hide();
        ui->ipfsLine->hide();
        ui->ipfsText->clear();
    }

    CheckFormState();
}

void CreateAssetDialog::showMessage(QString string)
{
    ui->messageLabel->setStyleSheet("color: red");
    ui->messageLabel->setText(string);
    ui->messageLabel->show();
    ui->availabilityButton->setDisabled(true);
    CheckFormState();
}

void CreateAssetDialog::showValidMessage(QString string)
{
    ui->messageLabel->setStyleSheet("color: green");
    ui->messageLabel->setText(string);
    ui->messageLabel->show();
    ui->availabilityButton->setDisabled(true);
    CheckFormState();
}

void CreateAssetDialog::hideMessage()
{
    ui->nameText->setStyleSheet("");
    ui->addressText->setStyleSheet("");
    if (ui->ipfsBox->isChecked())
        ui->ipfsText->setStyleSheet("");
    ui->messageLabel->hide();
}

void CreateAssetDialog::disableCreateButton()
{
    ui->createAssetButton->setDisabled(true);
}

void CreateAssetDialog::enableCreateButton()
{
    ui->createAssetButton->setDisabled(false);
}

void CreateAssetDialog::CheckFormState()
{
    disableCreateButton(); // Disable the button by default

    const CTxDestination dest = DecodeDestination(ui->addressText->text().toStdString());

    if (!(IsValidDestination(dest) || ui->addressText->text().isEmpty()) && IsAssetNameValid(ui->nameText->text().toStdString()))
        return;

    if (!IsAssetNameValid(ui->nameText->text().toStdString()))
        return;

    if (ui->ipfsBox->isChecked() && ui->ipfsText->text().size() != 46)
        return;

    enableCreateButton();
}

/** SLOTS */
void CreateAssetDialog::ipfsStateChanged()
{
    toggleIPFSText();
}

void CreateAssetDialog::checkAvailabilityClicked()
{
    QString name = ui->nameText->text();

    LOCK(cs_main);
    if (passets) {
        CNewAsset asset;
        if (passets->GetAssetIfExists(name.toStdString(), asset)) {
            ui->nameText->setStyleSheet("border: 1px solid red");
            showMessage("Invalid: Asset name already in use");
        } else {
            ui->nameText->setStyleSheet("border: 1px solid green");
            showValidMessage("Name is available");
        }
    } else {
        showMessage("Error: Asset Database not in sync");
    }
}

void CreateAssetDialog::onNameChanged(QString name)
{
    if (name.size() == 0) {
        hideMessage();
        return;
    }

    if (name.size() < 3) {
        ui->nameText->setStyleSheet("border: 1px solid red");
        showMessage("Invalid: Minimum of 3 character in length");
        return;
    }

    if (!IsAssetNameValid(name.toStdString())) {
        ui->nameText->setStyleSheet("border: 1px solid red");
        showMessage("Invalid: Allowed characters include: A-Z 0-9 . _");
    } else {
        hideMessage();
        ui->availabilityButton->setDisabled(false);
    }
}

void CreateAssetDialog::onAddressNameChanged(QString address)
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
    }
    else // Valid address
    {
        hideMessage();
        CheckFormState();
    }
}

void CreateAssetDialog::onIPFSHashChanged(QString hash)
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
        CheckFormState();
    }
}

void CreateAssetDialog::onCloseClicked()
{
    this->close();
}

void CreateAssetDialog::onCreateAssetClicked()
{
    QString address;
    if (ui->addressText->text().isEmpty()) {
        address = model->getAddressTableModel()->addRow(AddressTableModel::Receive, "", "");
    } else {
        address = ui->addressText->text();
    }

    QString name = ui->nameText->text();
    CAmount quantity = ui->quantitySpinBox->value() * COIN;
    int units = ui->unitBox->value();
    bool reissuable = ui->reissuableBox->isChecked();
    bool hasIPFS = ui->ipfsBox->isChecked();

    std::string ipfsDecoded = "";
    if (hasIPFS)
        ipfsDecoded = DecodeIPFS(ui->ipfsText->text().toStdString());

    CNewAsset asset(name.toStdString(), quantity, units, reissuable ? 1 : 0, hasIPFS ? 1 : 0, ipfsDecoded);

    // Create the transaction and broadcast it
    std::pair<int, std::string> error;
    std::string txid;
    if (!CreateAssetTransaction(model->getWallet(), asset, address.toStdString(), error, txid))
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
        msgBox.setText(tr("Created the asset transaction with the transaction id:"));
        msgBox.setInformativeText(QString::fromStdString(txid));
        msgBox.exec();

        if (msgBox.clickedButton() == okayButton) {
            close();
        }




    }
}

void CreateAssetDialog::onUnitChanged(int value)
{
    QString text;
    text += "e.g. 1";
    // Add the period
    if (value > 0)
        text += ".";

    // Add the remaining zeros
    for (int i = 0; i < value; i++) {
        text += "0";
    }

    ui->unitExampleLabel->setText(text);
}