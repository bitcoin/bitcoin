// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "createassetdialog.h"
#include "ui_createassetdialog.h"
#include "platformstyle.h"
#include "walletmodel.h"
#include "addresstablemodel.h"
#include "sendcoinsdialog.h"
#include "ravenunits.h"
#include "optionsmodel.h"

#include <QModelIndex>
#include <QDebug>
#include <validation.h>
#include <QMessageBox>
#include <script/standard.h>
#include <base58.h>
#include <QClipboard>
#include <wallet/wallet.h>
#include <core_io.h>
#include <policy/policy.h>

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
    connect(ui->rvnChangeBox, SIGNAL(clicked()), this, SLOT(onCustomAddressClicked()));
    connect(ui->changeAddressText, SIGNAL(textChanged(QString)), this, SLOT(onChangeAddressChanged(QString)));
    connect(ui->assetType, SIGNAL(activated(int)), this, SLOT(onAssetTypeActivated(int)));
    connect(ui->assetList, SIGNAL(activated(int)), this, SLOT(onAssetListActivated(int)));

    // Setup the default values
    setUpValues();

    format = "%1<font color=green>%2%3</font>";
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
    ui->changeAddressText->hide();
    ui->changeAddressLabel->hide();
    ui->changeAddressLine->hide();
    ui->availabilityButton->setDisabled(true);
    hideMessage();
    CheckFormState();

    ui->unitExampleLabel->setStyleSheet("font-weight: bold");

    // Setup the asset types
    QStringList list;
    list.append(tr("Main Asset"));
    list.append(tr("Sub Asset"));
//    list.append(tr("Unique Asset"));
    ui->assetType->addItems(list);
    type = ISSUE_ROOT;
    ui->assetTypeLabel->setText(tr("Asset Type") + ":");

    // Setup the asset list
    ui->assetList->hide();
    std::vector<std::string> names;
    GetAllAdministrativeAssets(model->getWallet(), names);
    for (auto item : names) {
        std::string name = QString::fromStdString(item).split("!").first().toStdString();
        if (name.size() != 30)
            ui->assetList->addItem(QString::fromStdString(name));
    }
    ui->assetFullName->setTextFormat(Qt::RichText);
    ui->assetFullName->setStyleSheet("font-weight: bold");

    ui->assetType->setStyleSheet("font-weight: bold");

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
}

void CreateAssetDialog::showValidMessage(QString string)
{
    ui->messageLabel->setStyleSheet("color: green");
    ui->messageLabel->setText(string);
    ui->messageLabel->show();
}

void CreateAssetDialog::hideMessage()
{
    ui->nameText->setStyleSheet("");
    ui->addressText->setStyleSheet("");
    if (ui->ipfsBox->isChecked())
        ui->ipfsText->setStyleSheet("");

    if (ui->rvnChangeBox->isChecked())
        ui->changeAddressText->setStyleSheet("");
    ui->messageLabel->hide();
}

void CreateAssetDialog::disableCreateButton()
{
    ui->createAssetButton->setDisabled(true);
}

void CreateAssetDialog::enableCreateButton()
{
    if (checkedAvailablity)
        ui->createAssetButton->setDisabled(false);
}

void CreateAssetDialog::CheckFormState()
{
    disableCreateButton(); // Disable the button by default
    hideMessage();

    const CTxDestination dest = DecodeDestination(ui->addressText->text().toStdString());

    QString name = GetAssetName();

    AssetType assetType;
    bool assetNameValid = IsAssetNameValid(name.toStdString(), assetType);
    if (assetNameValid && assetType == ROOT && type != ISSUE_ROOT)
        return;

    if (assetNameValid && assetType == SUB && type != ISSUE_SUB)
        return;

    if (assetNameValid && assetType == UNIQUE && type != ISSUE_UNIQUE)
        return;

    if (!(IsValidDestination(dest) || ui->addressText->text().isEmpty()) && assetNameValid) {
        ui->addressText->setStyleSheet("border: 1px solid red");
        showMessage("Warning: Invalid Raven address");
        return;
    }

    if (!assetNameValid)
        return;

    if(ui->rvnChangeBox->isChecked() && !IsValidDestinationString(ui->changeAddressText->text().toStdString())) {
        ui->changeAddressText->setStyleSheet("border: 1px solid red");
        showMessage(tr("If Custom Change Address is selected, a valid address must be given"));
        return;
    }

    if (ui->ipfsBox->isChecked() && ui->ipfsText->text().size() != 46) {
        ui->ipfsText->setStyleSheet("border: 1px solid red");
        showMessage("If JSON Meta Data is selected, a valid IPFS hash must be given");
        return;
    }

    if (checkedAvailablity) {
        showValidMessage("Valid Asset");
        enableCreateButton();
        ui->availabilityButton->setDisabled(true);
    } else {
        disableCreateButton();
        ui->availabilityButton->setDisabled(false);
    }
}

/** SLOTS */
void CreateAssetDialog::ipfsStateChanged()
{
    toggleIPFSText();
}

void CreateAssetDialog::checkAvailabilityClicked()
{
    QString name = GetAssetName();

    LOCK(cs_main);
    if (passets) {
        CNewAsset asset;
        if (passets->GetAssetIfExists(name.toStdString(), asset)) {
            ui->nameText->setStyleSheet("border: 1px solid red");
            showMessage("Invalid: Asset name already in use");
            disableCreateButton();
            checkedAvailablity = false;
            return;
        } else {
            checkedAvailablity = true;
            ui->nameText->setStyleSheet("border: 1px solid green");
        }
    } else {
        checkedAvailablity = false;
        showMessage("Error: Asset Database not in sync");
        disableCreateButton();
        return;
    }

    CheckFormState();
}

void CreateAssetDialog::onNameChanged(QString name)
{
    // Update the displayed name to uppercase if the type only accepts uppercase
    name = name.toUpper();
    UpdateAssetNameToUpper();

    QString assetName = name;

    // Get the identifier for the asset type
    QString identifier = GetSpecialCharacter();

    if (name.size() == 0) {
        hideMessage();
        ui->availabilityButton->setDisabled(true);
        updatePresentedAssetName(name);
        return;
    }

    if (type == ISSUE_ROOT) {
        if (name.size() < 3) {
            ui->nameText->setStyleSheet("border: 1px solid red");
            showMessage("Invalid: Minimum of 3 character in length");
            ui->availabilityButton->setDisabled(true);
            return;
        }

        AssetType assetType;
        if (IsAssetNameValid(name.toStdString(), assetType) && assetType == ROOT) {
            hideMessage();
            ui->availabilityButton->setDisabled(false);

        } else {
            ui->nameText->setStyleSheet("border: 1px solid red");
            showMessage("Invalid: Max Size 30 Characters. Allowed characters include: A-Z 0-9 . _");
        }
    } else if (type == ISSUE_SUB || type == ISSUE_UNIQUE) {
        if (name.size() == 0) {
            hideMessage();
            ui->availabilityButton->setDisabled(true);
            return;
        }

        AssetType assetType;
        if (IsAssetNameValid(ui->assetList->currentText().toStdString() + identifier.toStdString() + name.toStdString(), assetType) && (assetType == SUB || assetType == UNIQUE)) {
            hideMessage();
            ui->availabilityButton->setDisabled(false);
        } else {
            ui->nameText->setStyleSheet("border: 1px solid red");
            showMessage("Invalid: Max Size 30 Characters. Allowed characters include: A-Z 0-9 . _");
            ui->availabilityButton->setDisabled(true);
        }
    }

    // Set the assetName
    updatePresentedAssetName(format.arg(type == ISSUE_ROOT ? "" : ui->assetList->currentText(), identifier, name));

    checkedAvailablity = false;
    disableCreateButton();
}

void CreateAssetDialog::onAddressNameChanged(QString address)
{
    CheckFormState();
}

void CreateAssetDialog::onIPFSHashChanged(QString hash)
{
    CheckFormState();
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
    QString name = GetAssetName();

    CAmount quantity = ui->quantitySpinBox->value() * COIN;
    int units = ui->unitBox->value();
    bool reissuable = ui->reissuableBox->isChecked();
    bool hasIPFS = ui->ipfsBox->isChecked();

    std::string ipfsDecoded = "";
    if (hasIPFS)
        ipfsDecoded = DecodeIPFS(ui->ipfsText->text().toStdString());

    CNewAsset asset(name.toStdString(), quantity, units, reissuable ? 1 : 0, hasIPFS ? 1 : 0, ipfsDecoded);

    CWalletTx tx;
    CReserveKey reservekey(model->getWallet());
    std::pair<int, std::string> error;
    CAmount nFeeRequired;
    std::string changeAddress = ui->changeAddressText->text().toStdString();

    // Create the transaction
    if (!CreateAssetTransaction(model->getWallet(), asset, address.toStdString(), error, changeAddress, tx, reservekey, nFeeRequired)) {
        showMessage("Invalid: " + QString::fromStdString(error.second));
        return;
    }

    // Format confirmation message
    QStringList formatted;

    // generate bold amount string
    QString amount = "<b>" + QString::fromStdString(ValueFromAmountString(GetIssueAssetBurnAmount(), 8)) + " RVN";
    amount.append("</b>");
    // generate monospace address string
    QString addressburn = "<span style='font-family: monospace;'>" + QString::fromStdString(Params().IssueAssetBurnAddress());
    addressburn.append("</span>");

    QString recipientElement1;
    recipientElement1 = tr("%1 to %2").arg(amount, addressburn);
    formatted.append(recipientElement1);

    // generate the bold asset amount
    QString assetAmount = "<b>" + QString::fromStdString(ValueFromAmountString(asset.nAmount, asset.units)) + " " + QString::fromStdString(asset.strName);
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
    CAmount totalAmount = GetIssueAssetBurnAmount() + nFeeRequired;
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

    SendConfirmationDialog confirmationDialog(tr("Confirm send assets"),
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

void CreateAssetDialog::onCustomAddressClicked()
{
    if (ui->rvnChangeBox->isChecked()) {
        ui->changeAddressLabel->show();
        ui->changeAddressText->show();
        ui->changeAddressLine->show();
    } else {
        ui->changeAddressLabel->hide();
        ui->changeAddressText->hide();
        ui->changeAddressLine->hide();
        ui->changeAddressText->clear();
        hideMessage();
    }

    CheckFormState();
}

void CreateAssetDialog::onChangeAddressChanged(QString changeAddress)
{
    CheckFormState();
}

void CreateAssetDialog::onAssetTypeActivated(int index)
{
    // Update the selected type
    type = index;

    // Get the identifier for the asset type
    QString identifier = GetSpecialCharacter();

    if (index != 0) {
        ui->assetList->show();
    } else {
        ui->assetList->hide();
    }

    UpdateAssetNameMaxSize();

    // Set assetName
    updatePresentedAssetName(format.arg(type == ISSUE_ROOT ? "" : ui->assetList->currentText(), identifier, ui->nameText->text()));

    if (ui->nameText->text().size())
        ui->availabilityButton->setDisabled(false);
    else
        ui->availabilityButton->setDisabled(true);
    ui->createAssetButton->setDisabled(true);
}

void CreateAssetDialog::onAssetListActivated(int index)
{
    // Get the identifier for the asset type
    QString identifier = GetSpecialCharacter();

    UpdateAssetNameMaxSize();

    // Set assetName
    updatePresentedAssetName(format.arg(type == ISSUE_ROOT ? "" : ui->assetList->currentText(), identifier, ui->nameText->text()));

    if (ui->nameText->text().size())
        ui->availabilityButton->setDisabled(false);
    else
        ui->availabilityButton->setDisabled(true);
    ui->createAssetButton->setDisabled(true);
}

void CreateAssetDialog::updatePresentedAssetName(QString name)
{
    ui->assetFullName->setText(name);
}

QString CreateAssetDialog::GetSpecialCharacter()
{
    if (type == ISSUE_SUB)
        return "/";
    else if (type == ISSUE_UNIQUE)
        return "#";

    return "";
}

QString CreateAssetDialog::GetAssetName()
{
    if (type == ISSUE_ROOT)
        return ui->nameText->text();
    else if (type == ISSUE_SUB)
        return ui->assetList->currentText() + "/" + ui->nameText->text();
    else if (type == ISSUE_UNIQUE)
        return ui->assetList->currentText() + "#" + ui->nameText->text();
    return "";
}

void CreateAssetDialog::UpdateAssetNameMaxSize()
{
    if (type == ISSUE_ROOT) {
        ui->nameText->setMaxLength(30);
    } else if (type == ISSUE_SUB || type == ISSUE_UNIQUE) {
        ui->nameText->setMaxLength(30 - (ui->assetList->currentText().size() + 1));
    }
}

void CreateAssetDialog::UpdateAssetNameToUpper()
{
    if (type == ISSUE_ROOT || type == ISSUE_SUB) {
        ui->nameText->setText(ui->nameText->text().toUpper());
    }
}